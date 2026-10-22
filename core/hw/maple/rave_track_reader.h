// RAVE (TeamGT): reader of Local\RAVE_TRACK_V1 (rave_track.h) for the RAVE build of this core - the VR hands as RAVE
// publishes them. Windows only; elsewhere nothing is ever available. Header-only.
#pragma once

#include "rave_track.h"
#include <cstring>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

namespace rave_track
{
inline const volatile rave_track_block* block()
{
	static HANDLE mapping = nullptr;
	static const volatile rave_track_block* view = nullptr;
	static ULONGLONG next_try = 0;
	if (view)
		return view;
	const ULONGLONG now = GetTickCount64();
	if (now < next_try)
		return nullptr;
	next_try = now + 2000;  // RAVE makes it when it starts in VR: look again now and then
	mapping = OpenFileMappingW(FILE_MAP_READ, FALSE, RAVE_TRACK_NAME_W);
	if (!mapping)
		return nullptr;
	void* v = MapViewOfFile(mapping, FILE_MAP_READ, 0, 0, RAVE_TRACK_SIZE);
	if (!v)
	{
		CloseHandle(mapping);
		mapping = nullptr;
		return nullptr;
	}
	const volatile rave_track_block* b = static_cast<const volatile rave_track_block*>(v);
	if (b->magic != RAVE_TRACK_MAGIC || b->major != RAVE_TRACK_MAJOR || b->size < RAVE_TRACK_SIZE)
	{
		UnmapViewOfFile(v);
		CloseHandle(mapping);
		mapping = nullptr;
		return nullptr;
	}
	view = b;
	return view;
}

// A current copy of the whole block (seqlock + lease checked); false = RAVE isn't writing.
inline bool read(rave_track_block& out)
{
	const volatile rave_track_block* b = block();
	if (!b)
		return false;
	for (int attempt = 0; attempt < 2; attempt++)
	{
		const uint32_t before = b->sequence;
		MemoryBarrier();
		if (before & 1u)
			continue;
		std::memcpy(&out, const_cast<const rave_track_block*>(b), sizeof(out));
		MemoryBarrier();
		if (b->sequence != before)
			continue;
		return (out.flags & RAVE_TRACK_ACTIVE) && GetTickCount64() < out.written_ms + out.lease_ms;
	}
	return false;
}
}  // namespace rave_track
#else
namespace rave_track
{
inline bool read(rave_track_block&) { return false; }
}
#endif
