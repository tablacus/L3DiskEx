﻿/// @file basictype.cpp
///
/// @brief disk basic type
///
#include "basictype.h"
#include "basicfmt.h"
#include "basicdir.h"
#include "basicdiritem.h"
#include "logging.h"

//
//
//
DiskBasicTempData::DiskBasicTempData()
{
	memset(data, 0, sizeof(data));
	size = 0;
}

void DiskBasicTempData::SetData(const wxUint8 *data, size_t len)
{
	this->SetSize(len);
	memcpy(this->data, data, this->size);
}

///
/// DISK BASIC 個別の処理テンプレート
///
DiskBasicType::DiskBasicType(DiskBasic *basic, DiskBasicFat *fat, DiskBasicDir *dir)
{
	this->basic = basic;
	this->fat = fat;
	this->dir = dir;

	this->data_start_group = 0;
}
DiskBasicType::~DiskBasicType()
{
}

/// FAT位置をセット
/// @param [in] num グループ番号(0...)
/// @param [in] val 値
void DiskBasicType::SetGroupNumber(wxUint32 num, wxUint32 val)
{
}

/// FAT位置を返す
/// @param [in] num グループ番号(0...)
wxUint32 DiskBasicType::GetGroupNumber(wxUint32 num)
{
	return 0;
}

/// 使用しているグループ番号か
bool DiskBasicType::IsUsedGroupNumber(wxUint32 num)
{
	return true;
}

/// 次のグループ番号を得る
wxUint32 DiskBasicType::GetNextGroupNumber(wxUint32 num, int sector)
{
	return 0;
}

/// 空きFAT位置を返す
/// @return INVALID_GROUP_NUMBER: 空きなし
wxUint32 DiskBasicType::GetEmptyGroupNumber()
{
	wxUint32 new_num = INVALID_GROUP_NUMBER;
	// 若い番号順に検索
	for(wxUint32 num = 0; num <= end_group; num++) {
		wxUint32 gnum = GetGroupNumber(num);
		if (gnum == group_unused_code) {
			new_num = num;
			break;
		}
	}
	return new_num;
}

/// 次の空きFAT位置を返す
/// @return INVALID_GROUP_NUMBER: 空きなし
wxUint32 DiskBasicType::GetNextEmptyGroupNumber(wxUint32 curr_group)
{
	wxUint32 new_num = INVALID_GROUP_NUMBER;

	// 同じトラックでグループが連続するように検索
	int secs_per_grp = basic->GetSectorsPerGroup();
	int secs_per_trk = basic->GetSectorsOnBasic();
	int sides = basic->GetSidesOnBasic();

	int sed = secs_per_trk * sides / secs_per_grp;
	int group_max = (end_group / sed) + 1;
	int group_manage = managed_start_group / sed;
	int group_start = curr_group / sed;
	int group_end;
	int dir;
	int sst = 0; // curr_group % sed; // なるべく同じトラックを優先して埋める
	bool found = false;

	// 管理エリアより大きいグループは+方向、小さいグループなら-方向に検索
	dir = (group_start >= group_manage ? 1 : -1);

	for(int i=0; i<2; i++) {
		group_end = (dir > 0 ? group_max : -1);
		for(int g = group_start; g != group_end; g += dir) {
			for(int s = sst; s < sed; s++) {
				wxUint32 num = g * sed + s;
				if (num > end_group) {
					break;
				}
				wxUint32 gnum = GetGroupNumber(num);
//				myLog.SetDebug("DiskBasicType::GetNextEmptyGroupNumber num:0x%03x gnum:0x%03x", num, gnum);
				if (gnum == group_unused_code) {	// 0xff
					new_num = num;
					found = true;
					break;
				}
			}
			if (found) break;
			sst = 0;
		}
		if (found) break;
		dir = -dir;
		group_start = group_manage;
	}
	return new_num;
}

/// FATエリアをチェック
bool DiskBasicType::CheckFat()
{
	bool valid = true;

	wxUint32 end = end_group < 0xff ? end_group : 0xff;
	wxUint8 *tbl = new wxUint8[end + 1];
	memset(tbl, 0, end + 1);

	// 同じグループ番号が重複しているか
	for(wxUint32 pos = 0; pos <= end; pos++) {
		wxUint32 gnum = GetGroupNumber(pos);
		if (gnum <= end) {
			tbl[gnum]++;
		}
	}
	// 同じグループ番号が重複している場合エラー
	for(wxUint32 pos = 0; pos <= end; pos++) {
		if (tbl[pos] > 4) {
			valid = false;
			break;
		}
	}
	delete [] tbl;

	return valid;
}

/// 管理エリアのトラック番号からグループ番号を計算
wxUint32 DiskBasicType::CalcManagedStartGroup()
{
	int trk = basic->GetManagedTrackNumber();
	int sid = basic->GetFatSideNumber();
	if (sid < 0) sid = 0;
	int sides = basic->GetSidesOnBasic();
	int secs_per_grp = basic->GetSectorsPerGroup();
	int secs_per_trk = basic->GetSectorsOnBasic();
	managed_start_group = (trk * sides + sid) * secs_per_trk / secs_per_grp;
	return managed_start_group;
}

//

/// ルートディレクトリのチェック
bool DiskBasicType::CheckRootDirectory(int start_sector, int end_sector)
{
	bool valid = true;
	bool last = false;
	for(int sec_pos = start_sector; sec_pos <= end_sector && valid && !last; sec_pos++) {
		DiskD88Sector *sector = basic->GetManagedSector(sec_pos - 1, NULL, NULL);
		if (!sector) {
			valid = false;
			break;
		}
		wxUint8 *buffer = sector->GetSectorBuffer();
		if (!buffer) {
			valid = false;
			break;
		}
		int remain = sector->GetSectorSize();

		buffer += basic->GetDirStartPosOnSector();
		remain -= basic->GetDirStartPosOnSector();

		// ディレクトリのチェック
		while(valid && !last && remain > 0) {
			DiskBasicDirItem *nitem = dir->NewItem(sector, buffer);
			valid = nitem->Check(last);
			remain -= nitem->GetDataSize();
			buffer += nitem->GetDataSize();
			delete nitem;
		}
	}
	return valid;
}

/// ルートディレクトリをアサイン
bool DiskBasicType::AssignRootDirectory(int start_sector, int end_sector)
{
	int index_number = 0;
	bool unuse = false;
	for(int sec_pos = start_sector; sec_pos <= end_sector; sec_pos++) {
		int side_num, sec_num;
		DiskD88Track *track = basic->GetManagedTrack(sec_pos - 1, &side_num, &sec_num);
		if (!track) {
			continue;
		}
		DiskD88Sector *sector = track->GetSector(sec_num);
		if (!sector) break;

		int pos = 0;
		int size = sector->GetSectorSize();
		wxUint8 *buffer = sector->GetSectorBuffer();

		buffer += basic->GetDirStartPosOnSector();
		size -= basic->GetDirStartPosOnSector();

		while(pos < size) {
			DiskBasicDirItem *item = dir->AssignItem(index_number, track->GetTrackNumber(), side_num, sector, pos, buffer, unuse);
			pos += item->GetDataSize();
			buffer += item->GetDataSize();
			index_number++;
		}
	}

	return true;
}

/// ディレクトリのチェック
bool DiskBasicType::CheckDirectory(const DiskBasicGroups &group_items)
{
	bool valid = true;
	bool last = false;

	DiskBasicDirItem *nitem = dir->NewItem(NULL, NULL);
	for(size_t idx = 0; idx < group_items.Count(); idx++) {
		const DiskBasicGroupItem *gitem = group_items.ItemPtr(idx);
		int trk_num = gitem->track;
		int sid_num = gitem->side;
		DiskD88Track *track = basic->GetDisk()->GetTrack(trk_num, sid_num);
		if (!track) {
			valid = false;
			break;
		}
		for(int sec_num = gitem->sector_start; sec_num <= gitem->sector_end; sec_num++) {
			DiskD88Sector *sector = track->GetSector(sec_num);
			nitem->SetSector(sector);
			if (!sector) {
				valid = false;
				break;
			}
			wxUint8 *buffer = sector->GetSectorBuffer();
			if (!buffer) {
				valid = false;
				break;
			}
			int remain = sector->GetSectorSize();

			// ディレクトリのチェック
			while(valid && !last && remain > 0) {
				nitem->SetDataPtr((directory_t *)buffer);
				valid = nitem->Check(last);
				remain -= nitem->GetDataSize();
				buffer += nitem->GetDataSize();
			}
		}
	}
	delete nitem;
	return valid;
}

/// ディレクトリが空か
bool DiskBasicType::IsEmptyDirectory(const DiskBasicGroups &group_items)
{
	bool valid = true;
	bool last = false;

	DiskBasicDirItem *nitem = dir->NewItem(NULL, NULL);
	for(size_t idx = 0; idx < group_items.Count(); idx++) {
		const DiskBasicGroupItem *gitem = group_items.ItemPtr(idx);
		int trk_num = gitem->track;
		int sid_num = gitem->side;
		DiskD88Track *track = basic->GetDisk()->GetTrack(trk_num, sid_num);
		if (!track) {
			valid = false;
			break;
		}
		for(int sec_num = gitem->sector_start; sec_num <= gitem->sector_end && valid && !last; sec_num++) {
			DiskD88Sector *sector = track->GetSector(sec_num);
			nitem->SetSector(sector);
			if (!sector) {
				valid = false;
				break;
			}
			wxUint8 *buffer = sector->GetSectorBuffer();
			if (!buffer) {
				valid = false;
				break;
			}
			int remain = sector->GetSectorSize();

			// ディレクトリにファイルがないかのチェック
			while(valid && !last && remain > 0) {
				nitem->SetDataPtr((directory_t *)buffer);
				if (nitem->IsNormalFile()) {
					valid = !nitem->CheckUsed(last);
				}
				remain -= nitem->GetDataSize();
				buffer += nitem->GetDataSize();
			}
		}
	}
	delete nitem;
	return valid;
}

/// ディレクトリをアサイン
bool DiskBasicType::AssignDirectory(const DiskBasicGroups &group_items)
{
	int index_number = 0;
	bool unuse = false;
	int size_remain = (int)group_items.GetSize();
	for(size_t idx = 0; idx < group_items.Count(); idx++) {
		const DiskBasicGroupItem *gitem = group_items.ItemPtr(idx);
		int trk_num = gitem->track;
		int sid_num = gitem->side;
		DiskD88Track *track = basic->GetDisk()->GetTrack(trk_num, sid_num);
		if (!track) {
			continue;
		}
		for(int sec_num = gitem->sector_start; sec_num <= gitem->sector_end; sec_num++) {
			DiskD88Sector *sector = track->GetSector(sec_num);
			if (!sector) continue;

			int pos = 0;
			int size = sector->GetSectorSize();
			wxUint8 *buffer = sector->GetSectorBuffer();

			while(pos < size) {
				DiskBasicDirItem *item = dir->AssignItem(index_number, track->GetTrackNumber(), track->GetSideNumber(), sector, pos, buffer, unuse);
				if (size_remain <= 0) item->Used(false);
				pos += item->GetDataSize();
				buffer += item->GetDataSize();
				size_remain -= item->GetDataSize();
				index_number++;
			}
		}
	}

	return true;
}

//

/// 残りディスクサイズを計算
void DiskBasicType::CalcDiskFreeSize()
{
	wxUint32 fsize = 0;
	wxUint32 grps = 0;
	fat_availability.Empty();

	for(wxUint32 pos = 0; pos <= end_group; pos++) {
		wxUint32 gnum = GetGroupNumber(pos);
		int fsts = FAT_AVAIL_USED;
		if (gnum == group_unused_code) {
			fsize += (sector_size * secs_per_group);
			grps++;
			fsts = FAT_AVAIL_FREE;
		} else if (gnum == group_system_code) {
			fsts = FAT_AVAIL_SYSTEM;
		} else if (gnum >= group_final_code) {
			fsts = FAT_AVAIL_USED_LAST;
		}
		fat_availability.Add(fsts);
	}
	free_disk_size = (int)fsize;
	free_groups = (int)grps;
}

/// 残りディスクサイズをクリア
void DiskBasicType::ClearDiskFreeSize()
{
	free_disk_size = -1;
	free_groups = -1;
	fat_availability.Empty();
}

/// FATの空き状況を配列で返す
void DiskBasicType::GetFatAvailability(wxUint32 *offset, const wxArrayInt **arr) const
{
	*offset = 0;
	*arr = &fat_availability;
}

//

/// データサイズ分のグループを確保する
/// @return >0:正常 -1:空きなし(開始グループ設定前) -2:空きなし(開始グループ設定後)
int DiskBasicType::AllocateGroups(DiskBasicDirItem *item, int data_size, DiskBasicGroups &group_items)
{
//	myLog.SetDebug("DiskBasicType::AllocateGroups {");

//	int file_size = data_size;
	int groups = 0; 

	// FAT
	int  rc = 0;
	bool first_group = true;
	int sizeremain = data_size;
	int bytes_per_group = secs_per_group * sector_size;
	wxUint32 group_num = GetEmptyGroupNumber();
	int limit = end_group + 1;
	while(rc >= 0 && limit >= 0 && sizeremain > 0) {
		if (group_num == INVALID_GROUP_NUMBER) {
			// 空きなし
			rc = first_group ? -1 : -2;
			break;
		}
		// 位置を予約
		SetGroupNumber(group_num, basic->GetGroupFinalCode());

#if 0
		// トラック＆セクタがあるかをチェックする
		// グループ番号からトラック番号、サイド番号、セクタ番号を計算
		bool err_sector = false;
		DiskBasicGroups gitems;
		GetNumsFromGroup(gnum, 0, sector_size, sizeremain, gitems);
		for(int gidx = 0; gidx < (int)gitms.Count(); gidx++) {
			track_num = gitms[gidx].track;
			side_num = gitms[gidx].side;
			sector_start = gitms[gidx].sector_start;
			sector_end = gitms[gidx].sector_end;

			for(sector_num = sector_start; sector_num <= sector_end; sector_num++) {
				DiskD88Sector *sec = disk->GetSector(track_num, side_num, sector_num);
				if (!sec) {
					err_sector = true;
					break;
				}
			}
		}
		if (err_sector) {
			// セクタがないときの処理
			if (!type->SetSkipMarkOnErrorSector(item, prev_gnum, gnum, next_gnum)) {
				// エラーとする
				errinfo.SetError(DiskBasicError::ERR_NO_SECTOR, gitms[gidx].group, track_num, side_num, sector_num);
				// ディレクトリエントリを削除
				if (first_group) {
					item->Delete(GetDeleteCode());
				} else {
					this->DeleteItem(item, false);
				}
				return false;
			}
			gnum = next_gnum;
			continue;
		}
#endif

		// グループ番号の書き込み
		if (first_group) {
			item->SetStartGroup(group_num);
			first_group = false;
		}

		// 次の空きグループをさがす
		wxUint32 next_group_num = GetNextEmptyGroupNumber(group_num);

//		myLog.SetDebug("  group_num:0x%03x next:0x%03x", group_num, next_group_num);

		// 次の空きがない場合 or 残りサイズがこのグループで収まる場合
		if (next_group_num == INVALID_GROUP_NUMBER || sizeremain <= bytes_per_group) {
			// 最後のグループ番号
			next_group_num = CalcLastGroupNumber(next_group_num, sizeremain);
		}
		
		basic->GetNumsFromGroup(group_num, next_group_num, sector_size, sizeremain, group_items);

		// グループ番号設定
		SetGroupNumber(group_num, next_group_num);

//		prev_gnum = gnum;
		group_num = next_group_num;

		sizeremain -= bytes_per_group;
		groups++;

		limit--;
	}
	if (limit < 0) {
		// too large or infinit loop
		rc = first_group ? -1 : -2;
	}

//	myLog.SetDebug("rc: %d }", rc);
	return rc;
}

/// グループ番号から開始セクタ番号を得る
int DiskBasicType::GetStartSectorFromGroup(wxUint32 group_num)
{
	return group_num * secs_per_group;
}

/// グループ番号から最終セクタ番号を得る
int DiskBasicType::GetEndSectorFromGroup(wxUint32 group_num, wxUint32 next_group, int sector_start, int sector_size, int remain_size)
{
	int sector_end = sector_start + secs_per_group - 1;
	if (next_group >= group_final_code) {
		// 最終グループの場合指定したセクタまで
		sector_end = sector_start + (next_group - group_final_code);
	}
	return sector_end;
}

/// データ領域の開始セクタを計算
int DiskBasicType::CalcDataStartSectorPos()
{
	return 0;
}

/// スキップするトラック番号
int DiskBasicType::CalcSkippedTrack()
{
	return 0x7fff;
}

/// サイド番号を逆転するか
bool DiskBasicType::IsSideReversed(int sides_per_disk)
{
	return false;
}

/// ディスク内のデータが反転しているか
bool DiskBasicType::IsDataInverted()
{
	return false;
}

/// ルートディレクトリか
bool DiskBasicType::IsRootDirectory(wxUint32 group_num)
{
	return true;
}

//
// for format
//

/// セクタデータを指定コードで埋める
void DiskBasicType::FillSector(DiskD88Track *track, DiskD88Sector *sector)
{
	sector->Fill(basic->GetFillCodeOnFormat());
}

/// セクタデータを埋めた後の個別処理
void DiskBasicType::AdditionalProcessOnFormatted()
{
}

//
// for data access (read/verify)
//

/// ファイルの最終セクタのデータサイズを求める
int DiskBasicType::CalcDataSizeOnLastSector(DiskBasicDirItem *item, wxInputStream *istream, wxOutputStream *ostream, const wxUint8 *sector_buffer, int sector_size, int remain_size)
{
	return remain_size;
}

/// データの読み込み/比較処理
/// @return >=0 : 処理したサイズ  -1:比較不一致  -2:セクタがおかしい  
int DiskBasicType::AccessFile(DiskBasicDirItem *item, wxInputStream *istream, wxOutputStream *ostream, const wxUint8 *sector_buffer, int sector_size, int remain_size, int sector_num, int sector_end)
{
	if (remain_size <= sector_size) {
		// ファイルの最終セクタ
		sector_size = CalcDataSizeOnLastSector(item, istream, ostream, sector_buffer, sector_size, remain_size);
	}
	if (sector_size < 0) {
		// セクタなし
		return -2;
	}

	if (ostream) {
		// 書き出し
		temp.SetData(sector_buffer, sector_size);
		ostream->Write((const void *)temp.GetData(), temp.GetSize());
	}
	if (istream) {
		// 読み込んで比較
		temp.SetSize(sector_size);
		istream->Read((void *)temp.GetData(), temp.GetSize());

		if (memcmp(temp.GetData(), sector_buffer, temp.GetSize()) != 0) {
			// データが異なる
			return -1;
		}
	}
	return sector_size;
}

//
// for write
//

/// 最後のグループ番号を計算する
wxUint32 DiskBasicType::CalcLastGroupNumber(wxUint32 group_num, int size_remain)
{
	return group_num;
}

/// セーブ時にセクタがなかった時の処理
/// @return true:スキップする / false:エラーにする
bool DiskBasicType::SetSkipMarkOnErrorSector(DiskBasicDirItem *item, wxUint32 prev_group, wxUint32 group, wxUint32 next_group)
{
	return false;
}

/// データの書き込み処理
/// @param [in]	 item			ディレクトリアイテム
/// @param [in]	 istream		ストリームデータ
/// @param [out] buffer			セクタ内の書き込み先バッファ
/// @param [in]  size			書き込み先バッファサイズ
/// @param [in]  remain			残りのデータサイズ
/// @param [in]  sector_num		セクタ番号
/// @param [in]  group_num		現在のグループ番号
/// @param [in]  next_group		次のグループ番号
/// @param [in]  sector_end		最終セクタ番号
/// @return 書き込んだバイト数
int DiskBasicType::WriteFile(DiskBasicDirItem *item, wxInputStream &istream, wxUint8 *buffer, int size, int remain, int sector_num, wxUint32 group_num, wxUint32 next_group, int sector_end)
{
	bool need_eof_code = item->NeedCheckEofCode();

	int len = 0;
	if (remain <= size) {
		// 残り少ない
		if (remain < 0) remain = 0;
		if (need_eof_code) {
			// 最終は終端コード
			if (remain > 1) istream.Read((void *)buffer, remain - 1);
			if (remain > 0) buffer[remain - 1]=0x1a;
		} else {
			if (remain > 0) istream.Read((void *)buffer, remain);
		}
		if (size > remain) {
			// バッファの余りは0サプレス
			memset((void *)&buffer[remain], 0, size - remain);
		}
		len = remain;
	} else {
		// 継続
		istream.Read((void *)buffer, size);
		len = size;
	}
	return len;
}

//
// for delete
//

/// 指定したグループ番号のFAT領域を削除する
void DiskBasicType::DeleteGroupNumber(wxUint32 group_num)
{
	// FATに未使用コードを設定
	SetGroupNumber(group_num, group_unused_code);
}