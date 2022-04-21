﻿/// @file diskplainwriter.h
///
/// @brief べたディスクライター
///
/// @author Copyright (c) Sasaji. All rights reserved.
///

#ifndef _DISKPLAIN_WRITER_H_
#define _DISKPLAIN_WRITER_H_

#include "common.h"


class wxOutputStream;
class DiskWriter;
class DiskD88;
class DiskD88Disk;
class DiskResult;

/// べたディスクライター
class DiskPlainWriter
{
private:
	DiskWriter *dw;
	DiskResult *result;

	/// ディスク1つを保存
	int SaveDisk(DiskD88Disk *disk, int side_number, wxOutputStream *ostream);

public:
	DiskPlainWriter(DiskWriter *dw, DiskResult *result);
	~DiskPlainWriter();

	/// ストリームの内容をファイルに保存
	int SaveDisk(DiskD88 *image, int disk_number, int side_number, wxOutputStream *ostream);
};

#endif /* _DISKPLAIN_WRITER_H_ */
