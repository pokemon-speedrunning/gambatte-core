#include "newstate.h"
#include <cstring>
#include <algorithm>

namespace gambatte {

NewStateDummy::NewStateDummy()
: length(0)
{
}

void NewStateDummy::Save(void const */*ptr*/, size_t size, char const */*name*/) {
	length += size;
}

void NewStateDummy::Load(void */*ptr*/, size_t /*size*/, char const */*name*/) {
}

NewStateExternalBuffer::NewStateExternalBuffer(char *buffer, long maxlength)
: buffer(buffer)
, length(0)
, maxlength(maxlength)
{
}

void NewStateExternalBuffer::Save(void const *ptr, size_t size, char const */*name*/) {
	if (maxlength - length >= (long)size)
		std::memcpy(buffer + length, ptr, size);

	length += size;
}

void NewStateExternalBuffer::Load(void *ptr, size_t size, char const */*name*/) {
	char *dst = static_cast<char *>(ptr);
	if (maxlength - length >= (long)size)
		std::memcpy(dst, buffer + length, size);

	length += size;
}

NewStateExternalFunctions::NewStateExternalFunctions(const FPtrs *ff)
: Save_(ff->Save_)
, Load_(ff->Load_)
, EnterSection_(ff->EnterSection_)
, ExitSection_(ff->ExitSection_)
{
}

void NewStateExternalFunctions::Save(void const *ptr, size_t size, char const *name) {
	Save_(ptr, size, name);
}

void NewStateExternalFunctions::Load(void *ptr, size_t size, char const *name) {
	Load_(ptr, size, name);
}

void NewStateExternalFunctions::EnterSection(char const *name) {
	EnterSection_(name);
}

void NewStateExternalFunctions::ExitSection(char const *name) {
	ExitSection_(name);
}

}
