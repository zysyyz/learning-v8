// Copyright 2015 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "libplatform/libplatform.h"
#include "v8.h"

using namespace v8;

class ArrayBufferAllocator : public v8::ArrayBuffer::Allocator {
    public:
        virtual void* Allocate(size_t length) {
            void* data = AllocateUninitialized(length);
            return data == NULL ? data : memset(data, 0, length);
        }
        virtual void* AllocateUninitialized(size_t length) {
            return malloc(length);
        }
        virtual void Free(void* data, size_t) {
            free(data);
        }
};


int main(int argc, char* argv[]) {
    // Inline caching util? IC in the v8 source seems to refer to Inline Cachine
    V8::InitializeICU();
    // Now this is where the files 'natives_blob.bin' and snapshot_blob.bin' come into play. But what
    // are these bin files?
    // The JavaScript spec specifies a lot of built-in functionality which every V8 context must provide.
    // For example, you can run Math.PI and that will work in a JavaScript console/repl. The global object
    // and all the built-in functionality must be setup and initialized into the V8 heap. This can be time
    // consuming and affect runtime performance if this has to be done every time. The blobs above are prepared
    // snapshots that get directly deserialized into the heap to provide an initilized context.
    V8::InitializeExternalStartupData(argv[0]);
    // set up thread pool etc.
    Platform* platform = platform::CreateDefaultPlatform();
    // justs sets the platform created above.
    V8::InitializePlatform(platform);
    V8::Initialize();

    ArrayBufferAllocator allocator;
    Isolate::CreateParams create_params;
    create_params.array_buffer_allocator = &allocator;
    // An Isolate is an independant copy of the V8 runtime which includes its own heap.
    // To different Isolates can run in parallel and can be seen as entierly different
    // sandboxed instances of a V8 runtime.
    Isolate* isolate = Isolate::New(create_params);
    {
        Isolate::Scope isolate_scope(isolate);
        // Create a stack-allocated handle scope.
        // A container for handles. Instead of having to manage individual handles (like deleting) them
        // you can simply delete the handle scope.
        HandleScope handle_scope(isolate);

        // Inside an instance of V8 (an Isolate) you can have multiple unrelated JavaScript applications
        // running. JavaScript has global level stuff, and one application should not mess things up for
        // another running application. Context allow for each application not step on each others toes.
        Local<Context> context = Context::New(isolate);
        // a Local<SomeType> is held on the stack, and accociated with a handle scope. When the handle
        // scope is deleted the GC can deallocate the objects.

       // Enter the context for compiling and running the script.
        Context::Scope context_scope(context);

        // Create a string containing the JavaScript source code.
        Local<String> source = String::NewFromUtf8(isolate, "'Hello'", NewStringType::kNormal).ToLocalChecked();

        // Compile the source code.
        Local<Script> script = Script::Compile(context, source).ToLocalChecked();

        // Run the script to get the result.
        Local<Value> result = script->Run(context).ToLocalChecked();

        // Convert the result to an UTF8 string and print it.
        String::Utf8Value utf8(result);
        printf("%s\n", *utf8);
    }

    // Dispose the isolate and tear down V8.
    isolate->Dispose();
    V8::Dispose();
    V8::ShutdownPlatform();
    delete platform;
    return 0;
}
