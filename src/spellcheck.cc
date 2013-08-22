#include <node.h>
#include <stdlib.h>
#include <string.h>

#include "hunspell.hxx"

using namespace node;
using namespace v8;

struct Baton {
    uv_work_t request;
    Persistent<Function> callback;

    char *word;

    // Hunspell handle
    Hunspell *spell;

    bool isCorrect;
    char **suggestions;
    int numSuggest;
};

static Persistent<FunctionTemplate> constructor;

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

    static Handle<Value> New(const Arguments& args) {
      HandleScope scope;
      const char *affpath;
      const char *dpath;

      if (!args.IsConstructCall()) {
        return ThrowException(Exception::TypeError(
          String::New("Use `new` to create instances of this object.")
        ));
      }

      if (args.Length() != 2) {
        return ThrowException(Exception::TypeError(
          String::New("Expecting two arguments")
        ));
      } else if (!args[0]->IsString()) {
        return ThrowException(Exception::TypeError(
          String::New("First argument must be a string")
        ));
      } else if (!args[1]->IsString()) {
        return ThrowException(Exception::TypeError(
          String::New("Second argument must be a string")
        ));
      }

      String::Utf8Value affpathstr(args[0]->ToString());
      String::Utf8Value dpathstr(args[1]->ToString());
      affpath = (const char*)(*affpathstr);
      dpath = (const char*)(*dpathstr);

      SpellCheck* obj = new SpellCheck(affpath, dpath);
      obj->Wrap(args.This());

      return args.This();
    }

    static Handle<Value> Check(const Arguments& args) {
      HandleScope scope;
      SpellCheck* obj = ObjectWrap::Unwrap<SpellCheck>(args.This());

      if (!args[0]->IsString()) {
        return ThrowException(Exception::TypeError(
            String::New("First argument must be a string")));
      }
      if (!args[1]->IsFunction()) {
        return ThrowException(Exception::TypeError(
            String::New("Second argument must be a callback function")));
      }

      Local<Function> callback = Local<Function>::Cast(args[1]);

      String::Utf8Value str(args[0]->ToString());

      Baton* baton = new Baton();
      baton->request.data = baton;
      baton->callback = Persistent<Function>::New(callback);
      baton->word = strdup((const char*)*str);
      baton->spell = obj->spell;
      baton->numSuggest = 0;
      baton->suggestions = NULL;

      int status = uv_queue_work(uv_default_loop(),
                                 &baton->request,
                                 SpellCheck::CheckWork,
                                 (uv_after_work_cb)SpellCheck::CheckAfter);
      assert(status == 0);

      return Undefined();
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
      HandleScope scope;
      Baton* baton = static_cast<Baton*>(req->data);

      const unsigned int argc = 3;
      Local<Value> argv[argc];
      argv[0] = Local<Value>::New(Null());
      argv[1] = Local<Value>::New(Boolean::New(baton->isCorrect));
      if (baton->numSuggest > 0 && baton->suggestions != NULL) {
        Local<Array> suggestions = Array::New(baton->numSuggest);
        for (int i = 0; i < baton->numSuggest; ++i)
          suggestions->Set(i, String::New(baton->suggestions[i]));
        baton->spell->free_list(&(baton->suggestions), baton->numSuggest);
        argv[2] = suggestions;
      } else
        argv[2] = Local<Value>::New(Undefined());

      free(baton->word);

      TryCatch try_catch;
      baton->callback->Call(Context::GetCurrent()->Global(), argc, argv);
      if (try_catch.HasCaught())
        FatalException(try_catch);

      baton->callback.Dispose();
      delete baton;
    }

    static void Initialize(Handle<Object> target) {
      HandleScope scope;

      Local<FunctionTemplate> tpl = FunctionTemplate::New(New);
      Local<String> name = String::NewSymbol("SpellCheck");

      constructor = Persistent<FunctionTemplate>::New(tpl);
      constructor->InstanceTemplate()->SetInternalFieldCount(1);
      constructor->SetClassName(name);

      NODE_SET_PROTOTYPE_METHOD(constructor, "check", Check);

      target->Set(name, constructor->GetFunction());
    }
};

extern "C" {
  void init(Handle<Object> target) {
    HandleScope scope;
    SpellCheck::Initialize(target);
  }

  NODE_MODULE(spellcheck, init);
}
