#include <cstring>
#include <libplatform/libplatform.h>
#include "ScriptBindings.h"
#include "V8ScriptEnv.h"
#include "V8ScriptFunction.h"
#include "V8ScriptArguments.h"

bool _v8_initialized = false;
int V8ScriptEnv::s_count = 0;
std::unique_ptr<v8::Platform> V8ScriptEnv::s_platform;

V8ScriptEnv::V8ScriptEnv()
	: _initialized(false), _isolate(nullptr)
{
}

V8ScriptEnv::~V8ScriptEnv()
{
	this->Cleanup();
}

void V8ScriptEnv::Initialize()
{
	if (_initialized)
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
	rc.set_stack_limit((uint32_t *)(((uint64_t)&rc)/2));
	create_params.constraints = rc;

	// Create v8 isolate
	create_params.array_buffer_allocator = v8::ArrayBuffer::Allocator::NewDefaultAllocator();
	_isolate = v8::Isolate::New(create_params);
	
	// Create global object and persist it
	v8::HandleScope handle_scope(_isolate);
	v8::Local<v8::ObjectTemplate> global_tpl = v8::ObjectTemplate::New(_isolate);
	_global_tpl.Reset(_isolate, global_tpl);

	// Increment v8 environment counter
	V8ScriptEnv::s_count++;
	
	// Initialized
	_initialized = true;
}

void V8ScriptEnv::Cleanup(bool shutDown)
{
	if (!_initialized) {
		return;
	}

	// Clear persistent handles to function-constructors
	for (auto it = _constructorMap.begin(); it != _constructorMap.end(); ++it)
		it->second.Reset();
	_constructorMap.clear();

	// Clear persistent handles to the global object, and context
	_global.Reset();
	_global_tpl.Reset();
	_context.Reset();

	// Dispose of v8 isolate
	_isolate->Dispose();
	_isolate = nullptr;
	delete create_params.array_buffer_allocator;
	
	// Decrease v8 environment counter
	V8ScriptEnv::s_count--;
	_initialized = false;

	// Shutdown v8
	if (shutDown && V8ScriptEnv::s_count == 0)
	{
		// After this is run, you can no not reinitialize v8!
		v8::V8::Dispose();
		v8::V8::ShutdownPlatform();
		_v8_initialized = false;
	}
}

bool V8ScriptEnv::ParseErrors(v8::TryCatch *tryCatch)
{
	if (tryCatch->HasCaught())
	{
		// Fetch the v8 isolate and context
		v8::Isolate *isolate = this->Isolate();
		v8::Local<v8::Context> context = this->Context();
		
		v8::Handle<v8::Message> message = tryCatch->Message();
		if (!message.IsEmpty())
		{
			_lastScriptError.error = *v8::String::Utf8Value(isolate, tryCatch->Exception());
			_lastScriptError.filename = *v8::String::Utf8Value(isolate, message->GetScriptResourceName());
			_lastScriptError.error_line = *v8::String::Utf8Value(isolate, message->GetSourceLine(context).ToLocalChecked());
			_lastScriptError.lineno = message->GetLineNumber(context).ToChecked();
			_lastScriptError.startcol = message->GetStartColumn(context).ToChecked();
			_lastScriptError.endcol = message->GetEndColumn(context).ToChecked();
		}

		return true;
	}
	
	return false;
}

IScriptFunction * V8ScriptEnv::Compile(const std::string& name, const std::string& source)
{
	// Fetch the v8 isolate and context
	v8::Isolate *isolate = this->Isolate();
	v8::Local<v8::Context> context = this->Context();

	// Create a stack-allocated scope for v8 calls
	v8::Locker lock(isolate);
	v8::Isolate::Scope isolate_scope(isolate);
	v8::HandleScope handle_scope(isolate);

	// Create context with global template
	if (context.IsEmpty())
	{
		v8::Local<v8::ObjectTemplate> global_tpl = PersistentToLocal(isolate, _global_tpl);
		context = v8::Context::New(isolate, 0, global_tpl);
		_context.Reset(isolate, context);
		_global.Reset(isolate, context->Global());
	}

	// Enter the context for compiling and running the script.
	v8::Context::Scope context_scope(context);
	
	// Create a string containing the JavaScript source code.
	v8::Local<v8::String> sourceStr = v8::String::NewFromUtf8(isolate, source.c_str(), v8::NewStringType::kNormal).ToLocalChecked();

	// Compile the source code.
	v8::TryCatch try_catch(isolate);
	v8::ScriptOrigin origin(v8::String::NewFromUtf8(isolate, name.c_str(), v8::NewStringType::kNormal).ToLocalChecked());
	v8::Local<v8::Script> script;
	if (!v8::Script::Compile(context, sourceStr, &origin).ToLocal(&script)) {
		ParseErrors(&try_catch);
		return nullptr;
	}
	
	// Run the script to get the result.
	v8::Local<v8::Value> result;

	if (!script->Run(context).ToLocal(&result)) {
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
	if (_context.IsEmpty())
		function();
	else
	{
		v8::Context::Scope context_scope(Context());
		function();
	}
}

void V8ScriptEnv::TerminateExecution()
{
	assert(_isolate);
	_isolate->TerminateExecution();
}

bool V8ScriptEnv::SetConstructor(const std::string& key, v8::Local<v8::FunctionTemplate> func_tpl)
{
	auto it = _constructorMap.find(key);
	if (it != _constructorMap.end())
		return false;

	_constructorMap[key] = v8::Global<v8::FunctionTemplate>(Isolate(), func_tpl);
	return true;
}
