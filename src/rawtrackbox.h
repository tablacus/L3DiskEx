﻿/// @file rawtrackbox.h
///
/// @brief Rawトラックダイアログ
///

#ifndef _RAWTRACKBOX_H_
#define _RAWTRACKBOX_H_

#define RAWTRACKBOX_TRANS \
_("Required information entry is empty.") \
_("'%s' is invalid") \
_("Validation conflict") \
_("'%s' should only contain digits.")

#include "common.h"
#include <wx/dialog.h>

class wxTextCtrl;
class DiskD88Disk;

/// Rawパラメータボックス
class RawTrackBox : public wxDialog
{
private:
//	wxTextCtrl *txtOffset;

public:
	RawTrackBox(wxWindow* parent, wxWindowID id, int num, wxUint32 offset, DiskD88Disk *disk);

	enum {
		IDC_TEXT_OFFSET = 1,
	};

	/// @name functions
	//@{
	int ShowModal();
//	bool ValidateParam();
	//@}

	// event procedures
//	void OnOK(wxCommandEvent& event);

	// properties
//	int GetValue();

	wxDECLARE_EVENT_TABLE();
};

#endif /* _RAWTRACKBOX_H_ */

