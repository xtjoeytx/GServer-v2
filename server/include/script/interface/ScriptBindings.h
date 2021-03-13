#pragma once

#ifndef SCRIPTBINDINGS_H
#define SCRIPTBINDINGS_H

#ifdef NDEBUG
#define SCRIPTENV_D(...) do {} while(0)
#else
#define SCRIPTENV_D(...) printf(__VA_ARGS__)
#endif

#include "ScriptArguments.h"
#include "ScriptEnv.h"
#include "ScriptFunction.h"
#include "ScriptObject.h"
#include "ScriptUtils.h"

#endif
