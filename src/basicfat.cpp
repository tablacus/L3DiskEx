﻿/// @file basicfat.cpp
///
/// @brief disk basic fat
///
#include "basicfat.h"
#include "basicfmt.h"
#include "basictype.h"

//
//
//
DiskBasicGroupItem::DiskBasicGroupItem()
{
	group = 0;
	next = 0;
	track = 0;
	side = 0;
	sector_start = 0;
	sector_end = 0;
}
DiskBasicGroupItem::DiskBasicGroupItem(wxUint32 n_group, wxUint32 n_next, int n_track, int n_side, int n_start, int n_end)
{
	this->Set(n_group, n_next, n_track, n_side, n_start, n_end);
}
void DiskBasicGroupItem::Set(wxUint32 n_group, wxUint32 n_next, int n_track, int n_side, int n_start, int n_end)
{
	group = n_group;
	next = n_next;
	track = n_track;
	side = n_side;
	sector_start = n_start;
	sector_end = n_end;
}

#include <wx/arrimpl.cpp>
WX_DEFINE_OBJARRAY(DiskBasicGroupItems);

//
//
//
DiskBasicGroups::DiskBasicGroups()
{
	size = 0;
}
void DiskBasicGroups::Add(wxUint32 n_group, wxUint32 n_next, int n_track, int n_side, int n_start, int n_end)
{
	items.Add(DiskBasicGroupItem(n_group, n_next, n_track, n_side, n_start, n_end));
}
void DiskBasicGroups::Add(const DiskBasicGroupItem &item)
{
	items.Add(item);
}
void DiskBasicGroups::Empty()
{
	items.Empty();
	size = 0;
}
size_t DiskBasicGroups::Count() const
{
	return items.Count();
}
DiskBasicGroupItem &DiskBasicGroups::Last() const
{
	return items.Last();
}
DiskBasicGroupItem &DiskBasicGroups::Item(size_t idx) const
{
	return items.Item(idx);
}
DiskBasicGroupItem *DiskBasicGroups::ItemPtr(size_t idx) const
{
	return &items.Item(idx);
}

//
//
//
DiskBasicFatBuffer::DiskBasicFatBuffer()
{
	size = 0;
	buffer = NULL;
}
DiskBasicFatBuffer::DiskBasicFatBuffer(wxUint8 *newbuf, int newsize)
{
	size = newsize;
	buffer = newbuf;
}
void DiskBasicFatBuffer::Fill(wxUint8 code)
{
	if (buffer) {
		memset(buffer, code, size);
	}
}
void DiskBasicFatBuffer::Copy(const wxUint8 *buf, size_t len)
{
	if (buffer) {
		memcpy(buffer, buf, (int)len < size ? len : size);
	}
}

#include <wx/arrimpl.cpp>
WX_DEFINE_OBJARRAY(DiskBasicFatBuffers);
WX_DEFINE_OBJARRAY(DiskBasicFatArea);

//
//
//
DiskBasicFat::DiskBasicFat(DiskBasic *basic)
{
	this->basic = basic;
	this->type = NULL;
	this->Clear();
}
DiskBasicFat::~DiskBasicFat()
{
}
bool DiskBasicFat::Assign()
{
	int sector_num = basic->GetFatStartSector();
	int side_num = basic->GetFatSideNumber();

	DiskD88Disk *disk = basic->GetDisk();
	DiskD88Track *managed_track;
	if (side_num >= 0) {
		// トラック、サイド番号から計算
		managed_track = disk->GetTrack(basic->GetManagedTrackNumber(), side_num);
	} else {
		// セクタ番号の通し番号で計算
		managed_track = basic->GetManagedTrack(basic->GetReservedSectors(), &side_num, &sector_num);
	}
	if (!managed_track) {
		return false;
	}
	// セクタ位置を得る
	start = basic->GetSectorPosFromNum(basic->GetManagedTrackNumber(), side_num, sector_num);

//	sides = newsides;

	type =			basic->GetType();
	format_type =	basic->GetFormatType();
	count =			basic->GetNumberOfFats();
	size =			basic->GetSectorsPerFat();
	start_pos =		basic->GetFatStartPos();
//	grps_per_track = newgrpspertrk;
//	sector_size =	0;

//	group_final_code = basic->GetGroupFinalCode();
//	group_system_code = basic->GetGroupSystemCode();
//	group_unused_code = basic->GetGroupUnusedCode();

	if (type) {
//		type->SetGrpsPerTrack(grps_per_track);

		type->CalcManagedStartGroup();
	}

	// set buffer pointer for useful accessing
	bool valid = true;
	int start_sector = start;
	int end_sector = start + size - 1;
	for(int fat_num = 0; fat_num < count && valid; fat_num++) {
		DiskBasicFatBuffers fatbufs;
		for(int sec_num = start_sector; sec_num <= end_sector; sec_num++) {
			DiskD88Sector *sector = basic->GetSectorFromSectorPos(sec_num);
			if (!sector) {
				valid = false;
				break;
			}

			wxUint8 *buf = sector->GetSectorBuffer();
			int ssize = sector->GetSectorSize();

//			sector_size = ssize;
			if (sec_num == start_sector) {
				// 最初のセクタだけ開始位置がずれる
				buf += start_pos;
				ssize -= start_pos;
			}
			DiskBasicFatBuffer fatbuf(buf, ssize);
			fatbufs.Add(fatbuf);
		}
		bufs.Add(fatbufs);

		start_sector += size;
		end_sector += size;
	}

	if (valid) {
		valid = type->CheckFat();
	}

	return valid;
}
void DiskBasicFat::Clear()
{
	format_type = FORMAT_TYPE_NONE;
	count = 0;
	size = 0;
	start = 0;
	start_pos = 0;
//	grps_per_track = 0;
//	sector_size = 0;
//	group_final_code = 0;
//	group_system_code = 0;
//	group_unused_code = 0;

	bufs.Clear();
}
void DiskBasicFat::Empty()
{
	Clear();
}

/// FAT領域の最初のセクタの指定位置のデータを取得
wxUint8 DiskBasicFat::Get(int pos) const
{
	wxUint8 code = 0;
	DiskD88Sector *sector = basic->GetSectorFromSectorPos(start);
	if (sector) {
		wxUint8 *buf = sector->GetSectorBuffer();
		int size = sector->GetSectorBufferSize();
		if (buf && pos < size) {
			code = buf[pos];
		}
	}
	return code;
}

/// FAT領域の最初のセクタにデータを書く
void DiskBasicFat::Set(int pos, wxUint8 code)
{
	int start_sector = start;
	for(int fat_num = 0; fat_num < count; fat_num++) {
		DiskD88Sector *sector = basic->GetSectorFromSectorPos(start_sector);
		if (sector) {
			wxUint8 *buf = sector->GetSectorBuffer();
			int size = sector->GetSectorBufferSize();
			if (buf && pos < size) {
				buf[pos] = code;
			}
		}
		start_sector += size;
	}
}

/// FAT領域の最初のセクタにデータを書く
void DiskBasicFat::Copy(const wxUint8 *buf, size_t len)
{
	int start_sector = start;
	for(int fat_num = 0; fat_num < count; fat_num++) {
		DiskD88Sector *sector = basic->GetSectorFromSectorPos(start_sector);
		if (sector) {
			sector->Copy(buf, len);
		}
		start_sector += size;
	}
}

/// FAT領域を指定データで埋める
void DiskBasicFat::Fill(wxUint8 code)
{
	int start_sector = start;
	int end_sector = start + size - 1;
	for(int fat_num = 0; fat_num < count; fat_num++) {
		for(int sec_num = start_sector; sec_num <= end_sector; sec_num++) {
			DiskD88Sector *sector = basic->GetSectorFromSectorPos(sec_num);
			if (sector) {
				sector->Fill(code);
			}
		}
		start_sector += size;
		end_sector += size;
	}
}