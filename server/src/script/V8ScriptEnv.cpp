#include <libplatform/libplatform.h>
#include "ScriptRunError.h"
#include "V8ScriptEnv.h"
#include "V8ScriptFunction.h"
#include "V8ScriptArguments.h"

int V8ScriptEnv::s_count = 0;
std::unique_ptr<v8::Platform> V8ScriptEnv::s_platform;

V8ScriptEnv::V8ScriptEnv()
	: _initialized(false), _isolate(0)
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

	// Initialize V8.
	//v8::V8::InitializeICUDefaultLocation(argv[0]);
	v8::V8::InitializeExternalStartupData(".");

	// Initialize v8 if this is the first vm
	if (V8ScriptEnv::s_count == 0)
	{
		s_platform = v8::platform::NewDefaultPlatform();
		v8::V8::InitializePlatform(s_platform.get());
		v8::V8::Initialize();
	}
	
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

void V8ScriptEnv::Cleanup()
{
	if (!_initialized) {
		return;
	}

	for (int i = 0; i < CONSTRUCTOR_COUNT; i++) {
		_ctors[i].Reset();
	}

	_global.Reset();
	_global_tpl.Reset();
	_context.Reset();

	// Dispose of v8 isolate
	_isolate->Dispose();
	_isolate = 0;
	delete create_params.array_buffer_allocator;
	
	// Cleanup v8
	if (V8ScriptEnv::s_count == 1)
	{
		v8::V8::Dispose();
		v8::V8::ShutdownPlatform();
	}
	
	// Decrease v8 environment counter
	V8ScriptEnv::s_count--;
	_initialized = false;
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
			// TODO(joey): this throws a seg-fault, pretty sure this is the correct way to do it though. untested
			//_lastScriptError.error = *v8::String::Utf8Value(isolate, tryCatch->Exception());
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
	v8::Isolate::Scope isolate_scope(isolate);
	v8::HandleScope handle_scope(isolate);

	// Create context with global template
	if (context.IsEmpty()) {
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
		printf("Script compile error: %s\n", source.c_str());
		ParseErrors(&try_catch);
		return nullptr;
	}
	
	// Run the script to get the result.
	v8::Local<v8::Value> result;

	// not catching errors here?? need to fix. throws an error under Server::addNpc call
	if (!script->Run(context).ToLocal(&result)) {
		// TODO(joey): script execution errors
		printf("Script run error\n");
		ParseErrors(&try_catch);
		return nullptr;
	}

	assert(!try_catch.HasCaught());
	return new V8ScriptFunction(this, result.As<v8::Function>());
}
