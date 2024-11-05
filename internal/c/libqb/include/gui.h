//-----------------------------------------------------------------------------------------------------
//    ___  ____   __   _  _   ____  _____    ____ _   _ ___   _     _ _
//   / _ \| __ ) / /_ | || | |  _ \| ____|  / ___| | | |_ _| | |   (_) |__  _ __ __ _ _ __ _   _
//  | | | |  _ \| '_ \| || |_| |_) |  _|   | |  _| | | || |  | |   | | '_ \| '__/ _` | '__| | | |
//  | |_| | |_) | (_) |__   _|  __/| |___  | |_| | |_| || |  | |___| | |_) | | | (_| | |  | |_| |
//   \__\_\____/ \___/   |_| |_|   |_____|  \____|\___/|___| |_____|_|_.__/|_|  \__,_|_|   \__, |
//                                                                                         |___/
//  QB64-PE GUI Library
//  Powered by tinyfiledialogs (http://tinyfiledialogs.sourceforge.net)
//
//-----------------------------------------------------------------------------------------------------

#pragma once

#include <stdarg.h>
#include <stdint.h>

struct qbs;

void sub__guiNotifyPopup(qbs *qbsTitle, qbs *qbsMessage, qbs *qbsIconType, int32_t passed);
int32_t func__guiMessageBox(qbs *qbsTitle, qbs *qbsMessage, qbs *qbsDialogType, qbs *qbsIconType, int32_t nDefaultButton, int32_t passed);
void sub__guiMessageBox(qbs *qbsTitle, qbs *qbsMessage, qbs *qbsIconType, int32_t passed);
qbs *func__guiInputBox(qbs *qbsTitle, qbs *qbsMessage, qbs *qbsDefaultInput, int32_t passed);
qbs *func__guiSelectFolderDialog(qbs *qbsTitle, qbs *qbsDefaultPath, int32_t passed);
uint32_t func__guiColorChooserDialog(qbs *qbsTitle, uint32_t nDefaultRGB, int32_t passed);
qbs *func__guiOpenFileDialog(qbs *qbsTitle, qbs *qbsDefaultPathAndFile, qbs *qbsFilterPatterns, qbs *qbsSingleFilterDescription, int32_t nAllowMultipleSelects,
                             int32_t passed);
qbs *func__guiSaveFileDialog(qbs *qbsTitle, qbs *qbsDefaultPathAndFile, qbs *qbsFilterPatterns, qbs *qbsSingleFilterDescription, int32_t passed);

int gui_alert(const char *message, const char *title, const char *type);
bool gui_alert(const char *fmt, ...);
