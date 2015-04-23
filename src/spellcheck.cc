#include <node.h>
#include <stdlib.h>
#include <string.h>

#include "nan.h"
#include "hunspell.hxx"

using namespace node;
using namespace v8;

struct Baton {
    uv_work_t request;
    NanCallback *callback;

    char *word;

    // Hunspell handle
    Hunspell *spell;

    bool isCorrect;
    char **suggestions;
    int numSuggest;
};

static Persistent<Function> constructor;

class SpellCheck : public ObjectWrap {
  public:
    Hunspell *spell;

    SpellCheck(const char *affpath, const char *dpath) {
      spell = new Hunspell(affpath, dpath);
    }
    ~SpellCheck() {
      if (spell != NULL)
        delete spell;
      spell = NULL;
    }

    static NAN_METHOD(New) {
      NanScope();
      const char *affpath;
      const char *dpath;

      if (!args.IsConstructCall()) {
        return NanThrowError(
	        "Use `new` to create instances of this object.");
      }

      if (args.Length() != 2) {
        return NanThrowError("Expecting two arguments");
      } else if (!args[0]->IsString()) {
        return NanThrowError("First argument must be a string");
      } else if (!args[1]->IsString()) {
        return NanThrowError(
          "Second argument must be a string");
      }

      String::Utf8Value affpathstr(args[0]->ToString());
      String::Utf8Value dpathstr(args[1]->ToString());
      affpath = (const char*)(*affpathstr);
      dpath = (const char*)(*dpathstr);

      SpellCheck* obj = new SpellCheck(affpath, dpath);
      obj->Wrap(args.This());

      NanReturnValue(args.This());
    }

    static NAN_METHOD(Check) {
      NanScope();
      SpellCheck* obj = ObjectWrap::Unwrap<SpellCheck>(args.This());

      if (!args[0]->IsString()) {
        return NanThrowError("First argument must be a string");
      }
      if (!args[1]->IsFunction()) {
        return NanThrowError("Second argument must be a callback function");
      }

      Local<Function> callbackHandle = args[1].As<Function>();
      NanCallback *callback = new NanCallback(callbackHandle);

      String::Utf8Value str(args[0]->ToString());

      Baton* baton = new Baton();
      baton->request.data = baton;
      baton->callback = callback;
      baton->word = strdup((const char*)*str);
      baton->spell = obj->spell;
      baton->numSuggest = 0;
      baton->suggestions = NULL;

      int status = uv_queue_work(uv_default_loop(),
                                 &baton->request,
                                 SpellCheck::CheckWork,
                                 (uv_after_work_cb)SpellCheck::CheckAfter);
      assert(status == 0);

      NanReturnValue(NanUndefined());
    }

    static void CheckWork(uv_work_t* req) {
      Baton* baton = static_cast<Baton*>(req->data);

      int dp = baton->spell->spell(baton->word);
      if (dp)
        baton->isCorrect = true;
      else {
        baton->isCorrect = false;
        baton->numSuggest = baton->spell->suggest(&(baton->suggestions),
                                                  baton->word);
      }
    }

    static void CheckAfter(uv_work_t* req) {
      NanScope();
      Baton* baton = static_cast<Baton*>(req->data);

      const unsigned int argc = 3;
      Local<Value> argv[argc];
      argv[0] = NanNull();
      argv[1] = NanNew<Boolean>(baton->isCorrect);
      if (baton->numSuggest > 0 && baton->suggestions != NULL) {
        Local<Array> suggestions = NanNew<Array>(baton->numSuggest);
        for (int i = 0; i < baton->numSuggest; ++i)
          suggestions->Set(i, NanNew(baton->suggestions[i]));
        baton->spell->free_list(&(baton->suggestions), baton->numSuggest);
        argv[2] = suggestions;
      } else
        argv[2] = NanUndefined();

      free(baton->word);

      TryCatch try_catch;
      baton->callback->Call(argc, argv);
      if (try_catch.HasCaught())
        FatalException(try_catch);

      delete baton->callback;
      delete baton;
    }

    static void Initialize(Handle<Object> exports) {
      NanScope();

      Local<FunctionTemplate> tpl = NanNew<FunctionTemplate>(New);

      tpl->InstanceTemplate()->SetInternalFieldCount(1);
      tpl->SetClassName(NanNew("SpellCheck"));

      NODE_SET_PROTOTYPE_METHOD(tpl, "check", Check);

      NanAssignPersistent(constructor, tpl->GetFunction());
      exports->Set(NanNew("SpellCheck"), tpl->GetFunction());
    }
};

extern "C" {
  void init(Handle<Object> exports) {
    NanScope();
    SpellCheck::Initialize(exports);
  }

  NODE_MODULE(spellcheck, init);
}
