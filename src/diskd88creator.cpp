﻿/// @file diskd88creator.cpp
///
/// @brief D88ディスクイメージ作成
///
/// @author Copyright (c) Sasaji. All rights reserved.
///

#include "diskd88creator.h"
#include "diskparam.h"
#include "diskd88.h"
#include "diskresult.h"


//
//
//
DiskD88Creator::DiskD88Creator(const wxString &diskname, const DiskParam &param, bool write_protect, DiskD88File *file, DiskResult &result)
{
	this->diskname = diskname;
	this->param = &param;
	this->write_protect = write_protect;
	this->file = file;
	this->result = &result;
}

DiskD88Creator::~DiskD88Creator()
{
}

/// セクタデータの作成
wxUint32 DiskD88Creator::CreateSector(int track_number, int side_number, int sector_number, int sector_size, int sectors_per_track, bool single_density, DiskD88Track *track)
{
	DiskD88Sector *sector = new DiskD88Sector(track_number, side_number, sector_number, sector_size, sectors_per_track, single_density);
	track->Add(sector);

	// このセクタデータのサイズを返す
	return (wxUint32)sizeof(d88_sector_header_t) + sector->GetSectorBufferSize();
}

/// トラックデータの作成
wxUint32 DiskD88Creator::CreateTrack(int track_number, int side_number, int offset_pos, wxUint32 offset, DiskD88Disk *disk)
{
	DiskD88Track *track = new DiskD88Track(disk, track_number, side_number, offset_pos, param->GetInterleave());

	int sector_max = param->GetSectorsPerTrack();
	int sector_size = param->GetSectorSize();
	bool single_density = param->FindSingleDensity(track_number, side_number, &sector_max, &sector_size);

	int *sector_nums = new int[sector_max + 1];

	// interleave
	memset(sector_nums, 0, sizeof(int) * (sector_max + 1));
	int sector_pos = 0;
	int err = 0;
	for(int sector_number = 1; sector_number <= sector_max && err == 0; sector_number++) {
		if (sector_pos >= sector_max) {
			sector_pos -= sector_max;
			while (sector_nums[sector_pos] > 0) {
				sector_pos++;
				if (sector_pos >= sector_max) {
					// ?? error
					err = 1;
					result->SetError(DiskResult::ERR_INTERLEAVE);
					break;
				}
			}
		}
		sector_nums[sector_pos] = sector_number;
		sector_pos += param->GetInterleave();
	}

	// create sectors
	wxUint32 track_size = 0;
	for(sector_pos = 0; sector_pos < sector_max && result->GetValid() >= 0; sector_pos++) {
		int sector_offset = 0;
		if (param->IsReversible()) {
			// 裏返しできる(AB面あり)場合
			side_number = 0;
		}
		if (param->GetNumberingSector() == 1) {
			// 連番にする場合
			sector_offset = side_number * sector_max;
		}
		track_size += CreateSector(track_number, side_number, sector_nums[sector_pos] + sector_offset, sector_size, sector_max, single_density, track);
	}

	if (result->GetValid() >= 0) {
		// トラックを追加
		track->SetSize(track_size);
		disk->Add(track);
	} else {
		delete track;
	}

	delete [] sector_nums;
	return track_size;
}

/// ディスクデータの作成
wxUint32 DiskD88Creator::CreateDisk(int disk_number, short mod_flags)
{
	DiskD88Disk *disk = new DiskD88Disk(file, diskname, disk_number, *param, write_protect);

	// create tracks
	size_t create_size = 0;
	int track_num = 0;
	int side_num = 0;
	for(int pos = 0; pos < DISKD88_MAX_TRACKS && result->GetValid() >= 0; pos++) {
		disk->SetOffsetWithoutHeader(pos, (wxUint32)create_size);
		disk->SetMaxTrackNumber(pos);

		create_size += CreateTrack(track_num, side_num, pos, disk->GetOffset(pos), disk);

		side_num++;
		if (side_num >= param->GetSidesPerDisk()) {
			track_num++;
			side_num = 0;
		}
		if (track_num >= param->GetTracksPerSide()) {
			break;
		}
	}

	if (result->GetValid() >= 0) {
		// ディスクを追加
		if (param->GetBasicTypes().IsEmpty()) {
			// パラメータが手動設定のときはそれらしいテンプレートをさがす
			disk->CalcMajorNumber();
		} else {
			// テンプレートから設定
			disk->SetDiskParam(*param);
			disk->AllocDiskBasics();
		}
		disk->SetSizeWithoutHeader((wxUint32)create_size);
		file->Add(disk, mod_flags);
	} else {
		delete disk;
	}

	return (wxUint32)create_size;
}
/// ディスクイメージの新規作成
int DiskD88Creator::Create()
{
	CreateDisk(0, DiskD88File::MODIFY_NONE);
	return result->GetValid();
}
/// 新規作成して既存のイメージに追加
int DiskD88Creator::Add()
{
	int disk_number = 0;
	DiskD88Disks *disks = file->GetDisks();
	if (disks) {
		disk_number = (int)disks->Count();
	}

	CreateDisk(disk_number, DiskD88File::MODIFY_ADD);

	return result->GetValid();
}

