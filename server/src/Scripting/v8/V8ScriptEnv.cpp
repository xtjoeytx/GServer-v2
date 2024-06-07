#include "V8ScriptEnv.h"
#include "ScriptBindings.h"
#include "V8ScriptArguments.h"
#include "V8ScriptFunction.h"
#include <cstring>
#include <libplatform/libplatform.h>

bool _v8_initialized = false;
int V8ScriptEnv::s_count = 0;
std::unique_ptr<v8::Platform> V8ScriptEnv::s_platform;

V8ScriptEnv::V8ScriptEnv()
	: m_initialized(false), m_isolate(nullptr)
{
}

V8ScriptEnv::~V8ScriptEnv()
{
	this->Cleanup();
}

void V8ScriptEnv::Initialize()
{
	if (m_initialized)
		return;

	// Force v8 to use strict mode
	const char* flags = "--use_strict";
	v8::V8::SetFlagsFromString(flags, strlen(flags));

	// Initialize V8.
	v8::V8::InitializeICUDefaultLocation(".");
	v8::V8::InitializeExternalStartupData(".");

	// Initialize v8 if this is the first vm
	if (!_v8_initialized)
	{
		s_platform = v8::platform::NewDefaultPlatform();
		v8::V8::InitializePlatform(s_platform.get());
		v8::V8::Initialize();
		_v8_initialized = true;
	}

	// Sets the lower limit of the stack, as far as I know std::thread does not let me control the size
	//	of the stack, or lets me know how large it is. This fix seems to work for now, if it causes issues we can
	//	most-likely figure out what the default stack size is per thread and set the constraints through that.
	//	Fix from https://fw.hardijzer.nl/?p=97
	v8::ResourceConstraints rc;
	rc.set_stack_limit((uint32_t*)(((uint64_t)&rc) / 2));
	m_createParams.constraints = rc;

	// Create v8 isolate
	m_createParams.array_buffer_allocator = v8::ArrayBuffer::Allocator::NewDefaultAllocator();
	m_isolate = v8::Isolate::New(m_createParams);

	// Create global object and persist it
	v8::HandleScope handle_scope(m_isolate);
	v8::Local<v8::ObjectTemplate> global_tpl = v8::ObjectTemplate::New(m_isolate);
	m_globalTpl.Reset(m_isolate, global_tpl);

	// Increment v8 environment counter
	V8ScriptEnv::s_count++;

	// Initialized
	m_initialized = true;
}

void V8ScriptEnv::Cleanup(bool shutDown)
{
	if (!m_initialized)
	{
		return;
	}

	// Clear persistent handles to function-constructors
	for (auto& it: m_constructorMap)
		it.second.Reset();
	m_constructorMap.clear();

	// Clear persistent handles to the global object, and context
	m_global.Reset();
	m_globalTpl.Reset();
	m_context.Reset();

	// Dispose of v8 isolate
	m_isolate->Dispose();
	m_isolate = nullptr;
	delete m_createParams.array_buffer_allocator;

	// Decrease v8 environment counter
	V8ScriptEnv::s_count--;
	m_initialized = false;

	// Shutdown v8
	if (shutDown && V8ScriptEnv::s_count == 0)
	{
		// After this is run, you can not reinitialize v8!
		v8::V8::Dispose();
		v8::V8::ShutdownPlatform();
		_v8_initialized = false;
	}
}

bool V8ScriptEnv::ParseErrors(v8::TryCatch* tryCatch)
{
	if (tryCatch->HasCaught())
	{
		// Fetch the v8 isolate and context
		v8::Isolate* isolate = this->Isolate();
		v8::Local<v8::Context> context = this->Context();

		v8::Handle<v8::Message> message = tryCatch->Message();
		if (!message.IsEmpty())
		{
			m_lastScriptError.error = *v8::String::Utf8Value(isolate, tryCatch->Exception());
			m_lastScriptError.filename = *v8::String::Utf8Value(isolate, message->GetScriptResourceName());
			m_lastScriptError.error_line = *v8::String::Utf8Value(isolate, message->GetSourceLine(context).ToLocalChecked());
			m_lastScriptError.lineno = message->GetLineNumber(context).ToChecked();
			m_lastScriptError.startcol = message->GetStartColumn(context).ToChecked();
			m_lastScriptError.endcol = message->GetEndColumn(context).ToChecked();
		}

		return true;
	}

	return false;
}

IScriptFunction* V8ScriptEnv::Compile(const std::string& name, const std::string& source)
{
	// Fetch the v8 isolate and context
	v8::Isolate* isolate = this->Isolate();
	v8::Local<v8::Context> context = this->Context();

	// Create a stack-allocated scope for v8 calls
	v8::Locker lock(isolate);
	v8::Isolate::Scope isolate_scope(isolate);
	v8::HandleScope handle_scope(isolate);

	// Create context with global template
	if (context.IsEmpty())
	{
		v8::Local<v8::ObjectTemplate> global_tpl = PersistentToLocal(isolate, m_globalTpl);
		context = v8::Context::New(isolate, 0, global_tpl);
		m_context.Reset(isolate, context);
		m_global.Reset(isolate, context->Global());
	}

	// Enter the context for compiling and running the script.
	v8::Context::Scope context_scope(context);

	// Create a string containing the JavaScript source code.
	v8::Local<v8::String> sourceStr = v8::String::NewFromUtf8(isolate, source.c_str(), v8::NewStringType::kNormal).ToLocalChecked();

	// Compile the source code.
	v8::TryCatch try_catch(isolate);
	v8::ScriptOrigin origin(v8::String::NewFromUtf8(isolate, name.c_str(), v8::NewStringType::kNormal).ToLocalChecked());
	v8::Local<v8::Script> script;
	if (!v8::Script::Compile(context, sourceStr, &origin).ToLocal(&script))
	{
		ParseErrors(&try_catch);
		return nullptr;
	}

	// Run the script to get the result.
	v8::Local<v8::Value> result;

	if (!script->Run(context).ToLocal(&result))
	{
		ParseErrors(&try_catch);
		return nullptr;
	}

	assert(!try_catch.HasCaught());
	return new V8ScriptFunction(this, result.As<v8::Function>());
}

void V8ScriptEnv::CallFunctionInScope(std::function<void()> function)
{
	// Fetch the v8 isolate, and create a stack-allocated scope for v8 calls
	v8::Locker lock(Isolate());
	v8::Isolate::Scope isolate_scope(Isolate());
	v8::HandleScope handle_scope(Isolate());

	// Call function in context if we have one
	if (m_context.IsEmpty())
		function();
	else
	{
		v8::Context::Scope context_scope(Context());
		function();
	}
}

void V8ScriptEnv::TerminateExecution()
{
	assert(m_isolate);
	m_isolate->TerminateExecution();
}

bool V8ScriptEnv::SetConstructor(const std::string& key, v8::Local<v8::FunctionTemplate> func_tpl)
{
	auto it = m_constructorMap.find(key);
	if (it != m_constructorMap.end())
		return false;

	m_constructorMap[key] = v8::Global<v8::FunctionTemplate>(Isolate(), func_tpl);
	return true;
}
