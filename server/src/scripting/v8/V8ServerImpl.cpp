#ifdef V8NPCSERVER

	#include <cassert>
	#include <httplib.h>
	#include <v8.h>

	#include "NPC.h"
	#include "Player.h"
	#include "level/Level.h"
	#include "scripting/ScriptEngine.h"
	#include "scripting/v8/V8ScriptFunction.h"
	#include "scripting/v8/V8ScriptObject.h"

void Server_Function_HttpGet(const v8::FunctionCallbackInfo<v8::Value>& args)
{
	v8::Isolate* isolate = args.GetIsolate();

	// Check the number of arguments passed.
	if (args.Length() < 1)
	{
		isolate->ThrowException(v8::Exception::TypeError(
			v8::String::NewFromUtf8(isolate, "Wrong number of arguments").ToLocalChecked()));
		return;
	}

	// Check the argument types.
	if (!args[0]->IsString())
	{
		isolate->ThrowException(v8::Exception::TypeError(
			v8::String::NewFromUtf8(isolate, "Wrong arguments").ToLocalChecked()));
		return;
	}

	v8::String::Utf8Value url(isolate, args[0]);

	std::string urlAndQuery = *url;
	std::regex urlRegex("(https?://[^/]+)(/?.*)");
	std::smatch match;
	std::string onlyPath;
	std::string onlyUrl;

	if (std::regex_search(urlAndQuery, match, urlRegex) && match.size() == 3)
	{
		onlyUrl = match[1].str();
		onlyPath = match[2].str();
	}
	else
	{
		isolate->ThrowException(v8::Exception::Error(
			v8::String::NewFromUtf8(isolate, "Invalid url").ToLocalChecked()));
		return;
	}

	auto cli = httplib::Client(onlyUrl);
	cli.enable_server_certificate_verification(false);

	auto r = cli.Get(onlyPath);

	if (r->status < 200 || r->status >= 300)
	{
		isolate->ThrowException(v8::Exception::Error(
			v8::String::NewFromUtf8(isolate, to_string(r.error()).c_str()).ToLocalChecked()));
		return;
	}

	args.GetReturnValue().Set(v8::String::NewFromUtf8(isolate, r->body.c_str()).ToLocalChecked());
}

void Server_Function_HttpPost(const v8::FunctionCallbackInfo<v8::Value>& args)
{
	v8::Isolate* isolate = args.GetIsolate();

	// Check the number of arguments passed.
	if (args.Length() < 2)
	{
		isolate->ThrowException(v8::Exception::TypeError(
			v8::String::NewFromUtf8(isolate, "Wrong number of arguments").ToLocalChecked()));
		return;
	}

	// Check the argument types.
	if (!args[0]->IsString() || !args[1]->IsString())
	{
		isolate->ThrowException(v8::Exception::TypeError(
			v8::String::NewFromUtf8(isolate, "Wrong arguments").ToLocalChecked()));
		return;
	}

	v8::String::Utf8Value url(isolate, args[0]);
	v8::String::Utf8Value postData(isolate, args[1]);
	std::string urlAndQuery = *url;
	std::regex urlRegex("(https?://[^/]+)(/?.*)");
	std::smatch match;
	std::string onlyPath;
	std::string onlyUrl;

	if (std::regex_search(urlAndQuery, match, urlRegex) && match.size() == 3)
	{
		onlyUrl = match[1].str();
		onlyPath = match[2].str();
	}
	else
	{
		isolate->ThrowException(v8::Exception::Error(
			v8::String::NewFromUtf8(isolate, "Invalid url").ToLocalChecked()));
		return;
	}

	std::string contentTypeStr;
	if (args.Length() >= 3)
	{
		v8::String::Utf8Value contentType(isolate, args[2]);
		contentTypeStr = *contentType;
	}
	else
	{
		contentTypeStr = "application/json";
	}

	auto cli = httplib::Client(onlyUrl);
	cli.enable_server_certificate_verification(false);

	std::string postDataStr = *postData;

	auto r = cli.Post(onlyPath, postDataStr, contentTypeStr);

	if (r->status < 200 || r->status >= 300)
	{
		isolate->ThrowException(v8::Exception::Error(
			v8::String::NewFromUtf8(isolate, to_string(r.error()).c_str()).ToLocalChecked()));
		return;
	}

	args.GetReturnValue().Set(v8::String::NewFromUtf8(isolate, r->body.c_str()).ToLocalChecked());
}

void Server_Function_FindLevel(const v8::FunctionCallbackInfo<v8::Value>& args)
{
	v8::Isolate* isolate = args.GetIsolate();

	V8ENV_THROW_CONSTRUCTOR(args, isolate);
	V8ENV_THROW_ARGCOUNT(args, isolate, 1);

	v8::Local<v8::Context> context = args.GetIsolate()->GetCurrentContext();
	Server* serverObject = unwrapObject<Server>(args.This());

	// Find level from user input
	if (args[0]->IsString())
	{
		v8::String::Utf8Value levelName(isolate, args[0]->ToString(context).ToLocalChecked());
		auto levelObject = serverObject->getLevel(*levelName);
		if (levelObject != nullptr)
		{
			V8ScriptObject<Level>* v8_wrapped = static_cast<V8ScriptObject<Level>*>(levelObject->getScriptObject());
			args.GetReturnValue().Set(v8_wrapped->handle(isolate));
		}
	}
}

void Server_Function_CreateLevel(const v8::FunctionCallbackInfo<v8::Value>& args)
{
	v8::Isolate* isolate = args.GetIsolate();

	V8ENV_THROW_CONSTRUCTOR(args, isolate);
	V8ENV_THROW_ARGCOUNT(args, isolate, 2);

	v8::Local<v8::Context> context = args.GetIsolate()->GetCurrentContext();
	Server* serverObject = unwrapObject<Server>(args.This());

	// Create level from user input
	if (args[0]->IsInt32() && args[1]->IsString())
	{
		short fillTile = args[0]->Uint32Value(context).ToChecked();
		std::string levelName = *v8::String::Utf8Value(isolate, args[1]->ToString(context).ToLocalChecked());

		auto levelObject = Level::createLevel(fillTile, levelName);
		if (levelObject != nullptr)
		{
			V8ScriptObject<Level>* v8_wrapped = static_cast<V8ScriptObject<Level>*>(levelObject->getScriptObject());
			args.GetReturnValue().Set(v8_wrapped->handle(isolate));
		}
	}
}

void Server_Function_FindNPC(const v8::FunctionCallbackInfo<v8::Value>& args)
{
	v8::Isolate* isolate = args.GetIsolate();

	V8ENV_THROW_CONSTRUCTOR(args, isolate);
	V8ENV_THROW_ARGCOUNT(args, isolate, 1);

	v8::Local<v8::Context> context = args.GetIsolate()->GetCurrentContext();
	Server* serverObject = unwrapObject<Server>(args.This());

	// Find npc object from user input
	NPCPtr npcObject;
	if (args[0]->IsString())
	{
		v8::String::Utf8Value npcName(isolate, args[0]->ToString(context).ToLocalChecked());
		npcObject = serverObject->getNPCByName(*npcName);
	}
	else if (args[0]->IsInt32())
	{
		unsigned int npcId = args[0]->Uint32Value(context).ToChecked();
		npcObject = serverObject->getNPC(npcId);
	}

	// Set the return value as the handle from the wrapped object
	if (npcObject != nullptr)
	{
		V8ScriptObject<NPC>* v8_wrapped = static_cast<V8ScriptObject<NPC>*>(npcObject->getScriptObject());
		args.GetReturnValue().Set(v8_wrapped->handle(isolate));
	}
}

void Server_Function_FindPlayer(const v8::FunctionCallbackInfo<v8::Value>& args)
{
	v8::Isolate* isolate = args.GetIsolate();

	V8ENV_THROW_CONSTRUCTOR(args, isolate);
	V8ENV_THROW_ARGCOUNT(args, isolate, 1);

	// TODO(joey): second parameter could indicticate if it should skip rcs?

	v8::Local<v8::Context> context = args.GetIsolate()->GetCurrentContext();
	Server* serverObject = unwrapObject<Server>(args.This());

	// Find player object from user input
	PlayerPtr playerObject;
	if (args[0]->IsString())
	{
		v8::String::Utf8Value accountName(isolate, args[0]->ToString(context).ToLocalChecked());
		playerObject = serverObject->getPlayer(*accountName, PLTYPE_ANYCLIENT);
	}
	else if (args[0]->IsInt32())
	{
		unsigned int playerId = args[0]->Uint32Value(context).ToChecked();
		playerObject = serverObject->getPlayer(playerId, PLTYPE_ANYPLAYER);
	}

	// Set the return value as the handle from the wrapped object
	if (playerObject != nullptr)
	{
		V8ScriptObject<Player>* v8_wrapped = static_cast<V8ScriptObject<Player>*>(playerObject->getScriptObject());
		args.GetReturnValue().Set(v8_wrapped->handle(isolate));
	}
}


// Server Method: server.loadstring(str filename);
void Server_Function_LoadString(const v8::FunctionCallbackInfo<v8::Value>& args)
{
	v8::Isolate* isolate = args.GetIsolate();

	V8ENV_THROW_CONSTRUCTOR(args, isolate);
	V8ENV_THROW_ARGCOUNT(args, isolate, 1);

	v8::Local<v8::Context> context = args.GetIsolate()->GetCurrentContext();
	Server* serverObject = unwrapObject<Server>(args.This());

	PlayerPtr npcServer = serverObject->getNPCServer();
	if (npcServer && args[0]->IsString())
	{
		v8::String::Utf8Value filePath(isolate, args[0]->ToString(context).ToLocalChecked());
		const auto& folderRights = npcServer->getFolderRights();
		
		if (folderRights.hasPermission(*filePath, FilePermissions::Read))
		{
			CString fileData;
			if (fileData.load(serverObject->getServerPath(*filePath)))
			{
				auto result = v8::String::NewFromUtf8(isolate, fileData.text(), v8::NewStringType::kNormal, fileData.length());
				args.GetReturnValue().Set(result.ToLocalChecked());
			}
		}
	}
}

// Server Method: server.savestring(str filename, str filedata, bool append = false);
void Server_Function_SaveString(const v8::FunctionCallbackInfo<v8::Value>& args)
{
	v8::Isolate* isolate = args.GetIsolate();

	V8ENV_THROW_CONSTRUCTOR(args, isolate);
	V8ENV_THROW_MINARGCOUNT(args, isolate, 2);

	v8::Local<v8::Context> context = args.GetIsolate()->GetCurrentContext();
	Server* serverObject = unwrapObject<Server>(args.This());

	PlayerPtr npcServer = serverObject->getNPCServer();
	if (npcServer && args[0]->IsString() && args[1]->IsString())
	{
		v8::String::Utf8Value filePath(isolate, args[0]->ToString(context).ToLocalChecked());
		v8::String::Utf8Value fileData(isolate, args[1]->ToString(context).ToLocalChecked());
		const auto& folderRights = npcServer->getFolderRights();

		if (folderRights.hasPermission(*filePath, FilePermissions::Read))
		{
			auto path = serverObject->getServerPath(*filePath);

			CString data;
			if (args.Length() > 2 && args[2]->BooleanValue(isolate))
				data.load(path);

			data.write(*fileData, fileData.length(), false);
			args.GetReturnValue().Set(data.save(path));
		}
	}
}

// Server Method: server.setshootparams(str shootparams);
void Server_Function_SetShootParams(const v8::FunctionCallbackInfo<v8::Value>& args)
{
	v8::Isolate* isolate = args.GetIsolate();

	// Throw an exception on constructor calls for method functions
	V8ENV_THROW_CONSTRUCTOR(args, isolate);

	// Throw an exception if we don't receive the minimum 8 arguments
	V8ENV_THROW_MINARGCOUNT(args, isolate, 1);

	v8::Local<v8::Context> context = isolate->GetCurrentContext();

	if (args[0]->IsString())
	{
		V8ENV_SAFE_UNWRAP(args, Server, server);

		if (server == nullptr) return;

		CString shootParams;
		for (int i = 0; i < args.Length(); i++)
		{
			shootParams << (std::string)*v8::String::Utf8Value(isolate, args[i]->ToString(context).ToLocalChecked()) << "\n";
		}
		shootParams.gtokenizeI();

		server->setShootParams(shootParams.text());
	}
}

void Server_Function_SaveLog(const v8::FunctionCallbackInfo<v8::Value>& args)
{
	v8::Isolate* isolate = args.GetIsolate();

	V8ENV_THROW_CONSTRUCTOR(args, isolate);
	V8ENV_THROW_ARGCOUNT(args, isolate, 2);

	if (args[0]->IsString() && args[1]->IsString())
	{
		V8ENV_SAFE_UNWRAP(args, Server, serverObject);

		v8::Local<v8::Context> context = args.GetIsolate()->GetCurrentContext();
		v8::String::Utf8Value filename(isolate, args[0]->ToString(context).ToLocalChecked());
		v8::String::Utf8Value message(isolate, args[1]->ToString(context).ToLocalChecked());

		serverObject->logToFile(*filename, *message);
	}
}

void Server_Function_SendToNC(const v8::FunctionCallbackInfo<v8::Value>& args)
{
	v8::Isolate* isolate = args.GetIsolate();

	V8ENV_THROW_CONSTRUCTOR(args, isolate);
	V8ENV_THROW_MINARGCOUNT(args, isolate, 1);

	v8::Local<v8::Context> context = args.GetIsolate()->GetCurrentContext();

	std::string msg;
	for (int i = 0; i < args.Length(); i++)
	{
		v8::String::Utf8Value str(isolate, args[i]->ToString(context).ToLocalChecked());
		msg.append(*str).append(" ");
	}

	if (!msg.empty())
	{
		V8ENV_SAFE_UNWRAP(args, Server, serverObject);
		serverObject->sendToNC(msg);
	}
}

void Server_Function_SendToRC(const v8::FunctionCallbackInfo<v8::Value>& args)
{
	v8::Isolate* isolate = args.GetIsolate();

	V8ENV_THROW_CONSTRUCTOR(args, isolate);
	V8ENV_THROW_MINARGCOUNT(args, isolate, 1);

	v8::Local<v8::Context> context = args.GetIsolate()->GetCurrentContext();

	std::string msg;
	for (int i = 0; i < args.Length(); i++)
	{
		v8::String::Utf8Value str(isolate, args[i]->ToString(context).ToLocalChecked());
		msg.append(" ").append(*str);
	}

	if (!msg.empty())
	{
		V8ENV_SAFE_UNWRAP(args, Server, serverObject);
		serverObject->sendToRC(CString("[Server]:") << msg);
	}
}

// PROPERTY: server.timevar
void Server_Get_TimeVar(v8::Local<v8::String> prop, const v8::PropertyCallbackInfo<v8::Value>& info)
{
	V8ENV_SAFE_UNWRAP(info, Server, serverObject);

	unsigned int timevar = serverObject->getNWTime();
	info.GetReturnValue().Set(timevar);
}

// PROPERTY: server.timevar2
void Server_Get_TimeVar2(v8::Local<v8::String> prop, const v8::PropertyCallbackInfo<v8::Value>& info)
{
	auto timevar = (unsigned int)time(0);
	info.GetReturnValue().Set(timevar);
}

// PROPERTY: Server Flags
void Server_GetObject_Flags(v8::Local<v8::String> prop, const v8::PropertyCallbackInfo<v8::Value>& info)
{
	v8::Isolate* isolate = info.GetIsolate();
	v8::Local<v8::Context> context = isolate->GetCurrentContext();
	v8::Local<v8::Object> self = info.This();

	v8::Local<v8::String> internalProperty = v8::String::NewFromUtf8Literal(isolate, "_internalFlags", v8::NewStringType::kInternalized);
	if (self->HasRealNamedProperty(context, internalProperty).ToChecked())
	{
		info.GetReturnValue().Set(self->Get(context, internalProperty).ToLocalChecked());
		return;
	}

	V8ENV_SAFE_UNWRAP(info, Server, serverObject);

	// Grab external data
	v8::Local<v8::External> data = info.Data().As<v8::External>();
	auto* scriptEngine = static_cast<ScriptEngine*>(data->Value());
	auto* env = dynamic_cast<V8ScriptEnv*>(scriptEngine->getScriptEnv());

	// Find constructor
	v8::Local<v8::FunctionTemplate> ctor_tpl = env->getConstructor("server.flags");
	assert(!ctor_tpl.IsEmpty());

	// Create new instance
	v8::Local<v8::Object> new_instance = ctor_tpl->InstanceTemplate()->NewInstance(context).ToLocalChecked();
	new_instance->SetAlignedPointerInInternalField(0, serverObject);

	v8::PropertyAttribute propAttr = static_cast<v8::PropertyAttribute>(v8::PropertyAttribute::ReadOnly | v8::PropertyAttribute::DontDelete | v8::PropertyAttribute::DontEnum);
	self->DefineOwnProperty(context, internalProperty, new_instance, propAttr).FromJust();
	info.GetReturnValue().Set(new_instance);
}

void Server_Flags_Getter(v8::Local<v8::Name> property, const v8::PropertyCallbackInfo<v8::Value>& info)
{
	v8::Isolate* isolate = info.GetIsolate();
	v8::Local<v8::Object> self = info.This();
	Server* serverObject = unwrapObject<Server>(self);

	// Get property name
	v8::Local<v8::String> name = v8::Local<v8::String>::Cast(property);
	v8::String::Utf8Value utf8(isolate, name);

	// Get server flag with the property
	CString flagValue = serverObject->getFlag(*utf8);
	v8::Local<v8::String> strText = v8::String::NewFromUtf8(isolate, flagValue.text()).ToLocalChecked();
	info.GetReturnValue().Set(strText);
}

void Server_Flags_Setter(v8::Local<v8::Name> property, v8::Local<v8::Value> value, const v8::PropertyCallbackInfo<v8::Value>& info)
{
	v8::Isolate* isolate = info.GetIsolate();
	v8::Local<v8::Object> self = info.This();
	Server* serverObject = unwrapObject<Server>(self);

	// Get property name
	v8::Local<v8::String> name = v8::Local<v8::String>::Cast(property);
	v8::String::Utf8Value utf8(isolate, name);

	// Get new value
	v8::String::Utf8Value newValue(isolate, value);
	serverObject->setFlag(*utf8, *newValue, true);
}

void Server_Flags_Enumerator(const v8::PropertyCallbackInfo<v8::Array>& info)
{
	v8::Isolate* isolate = info.GetIsolate();
	v8::Local<v8::Context> context = isolate->GetCurrentContext();
	v8::Local<v8::Object> self = info.This();
	Server* serverObject = unwrapObject<Server>(self);

	// Get flags list
	auto& flagList = serverObject->getServerFlags();

	v8::Local<v8::Array> result = v8::Array::New(isolate, (int)flagList.size());

	int idx = 0;
	for (const auto& [flag, value]: flagList)
		result->Set(context, idx++, v8::String::NewFromUtf8(isolate, flag.c_str()).ToLocalChecked()).Check();

	info.GetReturnValue().Set(result);
}

// PROPERTY: server.npcs
void Server_GetArray_Npcs(v8::Local<v8::String> prop, const v8::PropertyCallbackInfo<v8::Value>& info)
{
	v8::Isolate* isolate = info.GetIsolate();
	v8::Local<v8::Context> context = isolate->GetCurrentContext();
	v8::Local<v8::Object> self = info.This();
	Server* serverObject = unwrapObject<Server>(self);

	// Get npcs list
	auto& npcList = serverObject->getNPCList();

	v8::Local<v8::Array> result = v8::Array::New(isolate, (int)npcList.size());

	int idx = 0;
	for (auto it = npcList.begin(); it != npcList.end(); ++it)
	{
		V8ScriptObject<NPC>* v8_wrapped = static_cast<V8ScriptObject<NPC>*>(it->second->getScriptObject());
		result->Set(context, idx++, v8_wrapped->handle(isolate)).Check();
	}

	info.GetReturnValue().Set(result);
}

// PROPERTY: server.players
void Server_GetArray_Players(v8::Local<v8::String> prop, const v8::PropertyCallbackInfo<v8::Value>& info)
{
	v8::Isolate* isolate = info.GetIsolate();
	v8::Local<v8::Context> context = isolate->GetCurrentContext();
	v8::Local<v8::Object> self = info.This();
	Server* serverObject = unwrapObject<Server>(self);

	// Get npcs list
	auto& playerList = serverObject->getPlayerList();

	v8::Local<v8::Array> result = v8::Array::New(isolate);

	int idx = 0;
	for (auto it = playerList.begin(); it != playerList.end(); ++it)
	{
		Player* pl = it->second.get();
		if (pl->isHiddenClient())
			continue;

		V8ScriptObject<Player>* v8_wrapped = static_cast<V8ScriptObject<Player>*>(pl->getScriptObject());
		result->Set(context, idx++, v8_wrapped->handle(isolate)).Check();
	}

	info.GetReturnValue().Set(result);
}

// PROPERTY: server.serverlist
void Server_GetArray_Serverlist(v8::Local<v8::String> prop, const v8::PropertyCallbackInfo<v8::Value>& info)
{
	v8::Isolate* isolate = info.GetIsolate();
	v8::Local<v8::Context> context = isolate->GetCurrentContext();
	v8::Local<v8::Object> self = info.This();
	Server* serverObject = unwrapObject<Server>(self);

	// Get npcs list
	auto& listserver = serverObject->getServerList();
	auto& serverList = listserver.getServerList();

	v8::Local<v8::Object> result = v8::Object::New(isolate);

	for (auto it = serverList.begin(); it != serverList.end(); ++it)
	{
		v8::Local<v8::String> key_servername = v8::String::NewFromUtf8(isolate, it->first.c_str(), v8::NewStringType::kNormal).ToLocalChecked();
		result->Set(context, key_servername, v8::Number::New(isolate, it->second)).Check();
	}

	info.GetReturnValue().Set(result);
}

void bindClass_Server(ScriptEngine* scriptEngine)
{
	// Retrieve v8 environment
	V8ScriptEnv* env = static_cast<V8ScriptEnv*>(scriptEngine->getScriptEnv());
	v8::Isolate* isolate = env->isolate();

	// External pointer
	v8::Local<v8::External> engine_ref = v8::External::New(isolate, scriptEngine);

	// Create V8 string for "server"
	v8::Local<v8::String> serverStr = v8::String::NewFromUtf8Literal(isolate, "server", v8::NewStringType::kInternalized);

	// Create constructor for class
	v8::Local<v8::FunctionTemplate> server_ctor = v8::FunctionTemplate::New(isolate);
	v8::Local<v8::ObjectTemplate> server_proto = server_ctor->PrototypeTemplate();

	server_ctor->SetClassName(serverStr);
	server_ctor->InstanceTemplate()->SetInternalFieldCount(1);

	// Method functions
	server_proto->Set(v8::String::NewFromUtf8Literal(isolate, "httpget"), v8::FunctionTemplate::New(isolate, Server_Function_HttpGet, engine_ref));
	server_proto->Set(v8::String::NewFromUtf8Literal(isolate, "httppost"), v8::FunctionTemplate::New(isolate, Server_Function_HttpPost, engine_ref));
	server_proto->Set(v8::String::NewFromUtf8Literal(isolate, "findlevel"), v8::FunctionTemplate::New(isolate, Server_Function_FindLevel, engine_ref));
	server_proto->Set(v8::String::NewFromUtf8Literal(isolate, "createlevel"), v8::FunctionTemplate::New(isolate, Server_Function_CreateLevel, engine_ref));
	server_proto->Set(v8::String::NewFromUtf8Literal(isolate, "findnpc"), v8::FunctionTemplate::New(isolate, Server_Function_FindNPC, engine_ref));
	server_proto->Set(v8::String::NewFromUtf8Literal(isolate, "findplayer"), v8::FunctionTemplate::New(isolate, Server_Function_FindPlayer, engine_ref));
	server_proto->Set(v8::String::NewFromUtf8Literal(isolate, "loadstring"), v8::FunctionTemplate::New(isolate, Server_Function_LoadString, engine_ref));
	server_proto->Set(v8::String::NewFromUtf8Literal(isolate, "savestring"), v8::FunctionTemplate::New(isolate, Server_Function_SaveString, engine_ref));
	server_proto->Set(v8::String::NewFromUtf8Literal(isolate, "setshootparams"), v8::FunctionTemplate::New(isolate, Server_Function_SetShootParams, engine_ref));
	server_proto->Set(v8::String::NewFromUtf8Literal(isolate, "savelog"), v8::FunctionTemplate::New(isolate, Server_Function_SaveLog, engine_ref));
	server_proto->Set(v8::String::NewFromUtf8Literal(isolate, "sendtonc"), v8::FunctionTemplate::New(isolate, Server_Function_SendToNC, engine_ref));
	server_proto->Set(v8::String::NewFromUtf8Literal(isolate, "sendtorc"), v8::FunctionTemplate::New(isolate, Server_Function_SendToRC, engine_ref));

	// Properties
	server_proto->SetAccessor(v8::String::NewFromUtf8Literal(isolate, "flags"), Server_GetObject_Flags, nullptr, engine_ref);
	server_proto->SetAccessor(v8::String::NewFromUtf8Literal(isolate, "npcs"), Server_GetArray_Npcs);
	server_proto->SetAccessor(v8::String::NewFromUtf8Literal(isolate, "players"), Server_GetArray_Players);
	server_proto->SetAccessor(v8::String::NewFromUtf8Literal(isolate, "serverlist"), Server_GetArray_Serverlist);
	server_proto->SetAccessor(v8::String::NewFromUtf8Literal(isolate, "timevar"), Server_Get_TimeVar);
	server_proto->SetAccessor(v8::String::NewFromUtf8Literal(isolate, "timevar2"), Server_Get_TimeVar2);

	// Create the server flags template
	v8::Local<v8::FunctionTemplate> server_flags_ctor = v8::FunctionTemplate::New(isolate);
	server_flags_ctor->SetClassName(v8::String::NewFromUtf8Literal(isolate, "flags"));
	server_flags_ctor->InstanceTemplate()->SetInternalFieldCount(1);
	server_flags_ctor->InstanceTemplate()->SetHandler(v8::NamedPropertyHandlerConfiguration(
		Server_Flags_Getter, Server_Flags_Setter, nullptr, nullptr, Server_Flags_Enumerator, v8::Local<v8::Value>(),
		v8::PropertyHandlerFlags::kOnlyInterceptStrings));
	env->setConstructor("server.flags", server_flags_ctor);

	// Persist the constructor
	env->setConstructor("server", server_ctor);
}

#endif
