#include <v8.h>
#include <node.h>
#include <node_buffer.h>

#include <IL/il.h>
#include <IL/ilu.h>

using namespace v8;

#define ArrayData(obj) \
    (obj->GetIndexedPropertiesExternalArrayData())

#define ArraySize(obj) \
    (obj->GetIndexedPropertiesExternalArrayDataLength() * Sizeof(obj))

static size_t Sizeof(Local<Object> object) {
    ExternalArrayType type = object->GetIndexedPropertiesExternalArrayDataType();
    switch (type) {
        case kExternalByteArray:
        case kExternalUnsignedByteArray:
            return 1;
        case kExternalShortArray:
        case kExternalUnsignedShortArray:
            return 2;
        case kExternalIntArray:
        case kExternalUnsignedIntArray:
        case kExternalFloatArray:
            return 4;
        case kExternalDoubleArray:
            return 8;
        case kExternalPixelArray:
            ThrowException(Exception::TypeError(String::New("Sizeof(PixelArray) unimplemented")));
    }
    ThrowException(Exception::TypeError(String::New("unknown ExternalArrayType")));
    return 0;
}

static ILubyte GetCount(ILenum format) {
    switch (format) {
        case IL_COLOUR_INDEX:
        case IL_LUMINANCE:
            return 1;
        case IL_RGB:
        case IL_BGR:
            return 3;
        case IL_RGBA:
        case IL_BGRA:
            return 4;
        default:
        ThrowException(String::New("unknown format"));
    }
    return 0;
}

static ILubyte GetBPP(ILenum format, ILenum type) {
    int size = 0;
    switch (type) {
        case IL_BYTE:
        case IL_UNSIGNED_BYTE:
            size = 1; break;
        case IL_SHORT:
        case IL_UNSIGNED_SHORT:
            size = 2; break;
        case IL_INT:
        case IL_UNSIGNED_INT:
        case IL_FLOAT:
            size = 4; break;
        case IL_DOUBLE:
            size = 8; break;
        default:
        ThrowException(String::New("unknown type"));
    }
    return GetCount(format) * size;
}

static ExternalArrayType GetType(ILenum type) {
    switch (type) {
        case IL_BYTE:
            return kExternalByteArray;
        case IL_UNSIGNED_BYTE:
            return kExternalUnsignedByteArray;
        case IL_SHORT:
            return kExternalShortArray;
        case IL_UNSIGNED_SHORT:
            return kExternalUnsignedShortArray;
        case IL_INT:
            return kExternalIntArray;
        case IL_UNSIGNED_INT:
            return kExternalUnsignedIntArray;
        case IL_FLOAT:
            return kExternalFloatArray;
        case IL_DOUBLE:
            return kExternalDoubleArray;
    }
    ThrowException(String::New("unknown type"));
    return kExternalByteArray;
}

static Handle<Value> Load(const Arguments& args) {
    HandleScope scope;
    Local<String> path = args[0]->ToString();
    Local<Object> image = Object::New();
    ILuint id;
    ilGenImages(1, &id);
    ilBindImage(id);
    ilLoadImage(*(String::AsciiValue)(path));
    ILuint width, height;
    ILenum format, type;
    ILubyte* data = ilGetData();
    ILubyte bpp;
    width = ilGetInteger(IL_IMAGE_WIDTH);
    height = ilGetInteger(IL_IMAGE_HEIGHT);
    format = ilGetInteger(IL_IMAGE_FORMAT);
    type = ilGetInteger(IL_IMAGE_TYPE);
    bpp = ilGetInteger(IL_IMAGE_BPP);

    node::Buffer *array = node::Buffer::New((char*)data, width*height*bpp);
    //Local<Object> array = Object::New();
//    array->SetIndexedPropertiesToExternalArrayData(
//        data,
//        GetType(type),
//        width*height*GetCount(format));
    image->Set(String::NewSymbol("width"), Integer::New(width));
    image->Set(String::NewSymbol("height"), Integer::New(height));
    image->Set(String::NewSymbol("format"), Integer::New(format));
    image->Set(String::NewSymbol("type"), Integer::New(type));
    image->Set(String::NewSymbol("data"), array->handle_);
    ilDeleteImages(1, &id);
    return scope.Close(image);
}

static Handle<Value> Save(const Arguments& args) {
    HandleScope scope;
    Local<String> path = args[0]->ToString();
    Local<Object> image = args[1]->ToObject();
    ILuint id;
    ilGenImages(1, &id);
    ilBindImage(id);
    ILuint width = image->Get(String::NewSymbol("width"))->IntegerValue();
    ILuint height = image->Get(String::NewSymbol("height"))->IntegerValue();
    ILenum format = image->Get(String::NewSymbol("format"))->IntegerValue();
    ILenum type = image->Get(String::NewSymbol("type"))->IntegerValue();
    ILubyte bpp = GetBPP(format, type);
    void* data;
    Local<Value> data_field = image->Get(String::NewSymbol("data"));
    if (data_field->IsUndefined() || data_field->IsNull()) {
        data = NULL;
    } else {
        data = ArrayData(data_field->ToObject());
        if (width*height*bpp > ArraySize(data_field->ToObject())) {
            ThrowException(String::New("data too small"));
        }
    }
    ilTexImage(width, height, 0, bpp, format, type, data);
    iluFlipImage(); // DevIL bug.
    ilSaveImage(*(String::AsciiValue)(path));
    ilDeleteImages(1, &id);
    return scope.Close(Undefined());
}

static void Init(Handle<Object> target) {
    ilInit(); iluInit();
    if (ilGetInteger(IL_VERSION_NUM) < IL_VERSION ||
        iluGetInteger(ILU_VERSION_NUM) < ILU_VERSION) {
        ThrowException(String::New("DevIL version is different"));
    }
    ilEnable(IL_FILE_OVERWRITE);
    target->Set(String::NewSymbol("loadSync"),
        FunctionTemplate::New(Load)->GetFunction());
    target->Set(String::NewSymbol("saveSync"),
        FunctionTemplate::New(Save)->GetFunction());
    target->Set(String::NewSymbol("COLOUR_INDEX"), Integer::New(IL_COLOUR_INDEX));
    target->Set(String::NewSymbol("LUMINANCE"), Integer::New(IL_LUMINANCE));
    target->Set(String::NewSymbol("RGB"), Integer::New(IL_RGB));
    target->Set(String::NewSymbol("BGR"), Integer::New(IL_BGR));
    target->Set(String::NewSymbol("RGBA"), Integer::New(IL_RGBA));
    target->Set(String::NewSymbol("BGRA"), Integer::New(IL_BGRA));

    target->Set(String::NewSymbol("BYTE"), Integer::New(IL_BYTE));
    target->Set(String::NewSymbol("UNSIGNED_BYTE"), Integer::New(IL_UNSIGNED_BYTE));
    target->Set(String::NewSymbol("SHORT"), Integer::New(IL_SHORT));
    target->Set(String::NewSymbol("UNSIGNED_SHORT"), Integer::New(IL_UNSIGNED_SHORT));
    target->Set(String::NewSymbol("INT"), Integer::New(IL_INT));
    target->Set(String::NewSymbol("UNSIGNED_INT"), Integer::New(IL_UNSIGNED_INT));
    target->Set(String::NewSymbol("FLOAT"), Integer::New(IL_FLOAT));
    target->Set(String::NewSymbol("DOUBLE"), Integer::New(IL_DOUBLE));
}
NODE_MODULE(openil, Init)
