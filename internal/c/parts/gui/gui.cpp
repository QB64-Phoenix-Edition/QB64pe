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

#include "libqb-common.h"

#include "qbs.h"
#include "gui.h"
#include "image.h"
#include "tinyfiledialogs.h"
#include <algorithm>
#include <string>
#include <string.h>
#include <limits.h>

/// @brief Splits a string delimited by '|' into an array of strings
/// @param input The string to be parsed
/// @param count Point to an integer that will hold the count of tokens. This cannot be NULL
/// @return Array of string tokens. This must be freed using gui_free_tokens()
static char **gui_tokenize(const char *input, int32_t *count) {
    auto str = strdup(input);
    auto capacity = 2;
    char **result = (char **)malloc(capacity * sizeof(*result));
    char *saveptr;

    *count = 0;
    auto tok = strtok_r(str, "|", &saveptr);

    for (;;) {
        if (*count >= capacity)
            result = (char **)realloc(result, (capacity *= 2) * sizeof(*result));

        result[*count] = tok ? strdup(tok) : tok;

        if (!tok)
            break;

        ++(*count);

        tok = strtok_r(nullptr, "|", &saveptr);
    }

    free(str);

    return result;
}

/// @brief Frees all string and the array itself created by gui_tokenize()
/// @param tokens Array of string pointers
static void gui_free_tokens(char **tokens) {
    for (auto it = tokens; it && *it; ++it)
        free(*it);

    free(tokens);
}

/// @brief Shows a system notification (on Windows this will be an action center notification)
/// @param qbsTitle [OPTIONAL] Title of the notification
/// @param qbsMessage [OPTIONAL] The message that will be displayed
/// @param qbsIconType [OPTIONAL] Icon type ("info" "warning" "error")
/// @param passed Optional parameter mask
void sub__guiNotifyPopup(qbs *qbsTitle, qbs *qbsMessage, qbs *qbsIconType, int32_t passed) {
    std::string aTitle;
    std::string aMessage;
    std::string aIconType;

    if (passed & 1)
        aTitle.assign((const char *)qbsTitle->chr, qbsTitle->len);

    if (passed & 2)
        aMessage.assign((const char *)qbsMessage->chr, qbsMessage->len);

    if (passed & 4) {
        aIconType.assign((const char *)qbsIconType->chr, qbsIconType->len);
        std::transform(aIconType.begin(), aIconType.end(), aIconType.begin(), [](unsigned char c) { return std::tolower(c); });
    } else {
        aIconType.assign("info");
    }

    tinyfd_notifyPopup(aTitle.c_str(), aMessage.c_str(), aIconType.c_str());
}

/// @brief Shows a standard system message dialog box
/// @param qbsTitle [OPTIONAL] Title of the dialog box
/// @param qbsMessage [OPTIONAL] The message that will be displayed
/// @param qbsIconType [OPTIONAL] The dialog icon type ("info" "warning" "error")
/// @param passed Optional parameter mask
void sub__guiMessageBox(qbs *qbsTitle, qbs *qbsMessage, qbs *qbsIconType, int32_t passed) {
    std::string aTitle;
    std::string aMessage;
    std::string aIconType;

    if (passed & 1)
        aTitle.assign((const char *)qbsTitle->chr, qbsTitle->len);

    if (passed & 2)
        aMessage.assign((const char *)qbsMessage->chr, qbsMessage->len);

    if (passed & 4) {
        aIconType.assign((const char *)qbsIconType->chr, qbsIconType->len);
        std::transform(aIconType.begin(), aIconType.end(), aIconType.begin(), [](unsigned char c) { return std::tolower(c); });
    } else {
        aIconType.assign("info");
    }

    tinyfd_messageBox(aTitle.c_str(), aMessage.c_str(), "ok", aIconType.c_str(), 1);
}

/// @brief Shows a standard system message dialog box
/// @param qbsTitle [OPTIONAL] Title of the dialog box
/// @param qbsMessage [OPTIONAL] The message that will be displayed
/// @param qbsDialogType [OPTIONAL] The dialog type ("ok" "okcancel" "yesno" "yesnocancel")
/// @param qbsIconType [OPTIONAL] The dialog icon type ("info" "warning" "error" "question")
/// @param nDefaultButon [OPTIONAL] The default button that will be selected
/// @param passed Optional parameter mask
/// @return 0 for cancel/no, 1 for ok/yes, 2 for no in yesnocancel
int32_t func__guiMessageBox(qbs *qbsTitle, qbs *qbsMessage, qbs *qbsDialogType, qbs *qbsIconType, int32_t nDefaultButton, int32_t passed) {
    std::string aTitle;
    std::string aMessage;
    std::string aDialogType;
    std::string aIconType;

    if (passed & 1)
        aTitle.assign((const char *)qbsTitle->chr, qbsTitle->len);

    if (passed & 2)
        aMessage.assign((const char *)qbsMessage->chr, qbsMessage->len);

    if (passed & 4) {
        aDialogType.assign((const char *)qbsDialogType->chr, qbsDialogType->len);
        std::transform(aDialogType.begin(), aDialogType.end(), aDialogType.begin(), [](unsigned char c) { return std::tolower(c); });
    } else {
        aDialogType.assign("ok");
    }

    if (passed & 8) {
        aIconType.assign((const char *)qbsIconType->chr, qbsIconType->len);
        std::transform(aIconType.begin(), aIconType.end(), aIconType.begin(), [](unsigned char c) { return std::tolower(c); });
    } else {
        aIconType.assign("info");
    }

    if (!(passed & 16))
        nDefaultButton = 1;

    return tinyfd_messageBox(aTitle.c_str(), aMessage.c_str(), aDialogType.c_str(), aIconType.c_str(), nDefaultButton);
}

/// @brief Shows an input box for getting a string from the user
/// @param qbsTitle [OPTIONAL] Title of the dialog box
/// @param qbsMessage [OPTIONAL] The message or prompt that will be displayed
/// @param qbsDefaultInput [OPTIONAL] The default response that can be changed by the user
/// @param passed Optional parameter mask
/// @return The user response or an empty string if the user cancelled
qbs *func__guiInputBox(qbs *qbsTitle, qbs *qbsMessage, qbs *qbsDefaultInput, int32_t passed) {
    std::string aTitle;
    std::string aMessage;
    std::string aDefaultInput;

    if (passed & 1)
        aTitle.assign((const char *)qbsTitle->chr, qbsTitle->len);

    if (passed & 2)
        aMessage.assign((const char *)qbsMessage->chr, qbsMessage->len);

    const char *sDefaultInput;
    if (passed & 4) {
        aDefaultInput.assign((const char *)qbsDefaultInput->chr, qbsDefaultInput->len);
        sDefaultInput = !qbsDefaultInput->len ? nullptr : aDefaultInput.c_str(); // if string is "" then password box, else we pass the default input as is
    } else {
        sDefaultInput = aDefaultInput.c_str();
    }

    auto sInput = tinyfd_inputBox(aTitle.c_str(), aMessage.c_str(), sDefaultInput);

    // Create a new qbs and then copy the string to it
    auto qbsInput = qbs_new(sInput ? strlen(sInput) : 0, 1);
    if (qbsInput->len)
        memcpy(qbsInput->chr, sInput, qbsInput->len);

    return qbsInput;
}

/// @brief Shows the browse for folder dialog box
/// @param qbsTitle [OPTIONAL] Title of the dialog box
/// @param qbsDefaultPath [OPTIONAL] The default path from where to start browsing
/// @param passed Optional parameter mask
/// @return The path selected by the user or an empty string if the user cancelled
qbs *func__guiSelectFolderDialog(qbs *qbsTitle, qbs *qbsDefaultPath, int32_t passed) {
    std::string aTitle;
    std::string aDefaultPath;

    if (passed & 1)
        aTitle.assign((const char *)qbsTitle->chr, qbsTitle->len);

    if (passed & 2)
        aDefaultPath.assign((const char *)qbsDefaultPath->chr, qbsDefaultPath->len);

    auto sFolder = tinyfd_selectFolderDialog(aTitle.c_str(), aDefaultPath.c_str());

    // Create a new qbs and then copy the string to it
    auto qbsFolder = qbs_new(sFolder ? strlen(sFolder) : 0, 1);
    if (qbsFolder->chr)
        memcpy(qbsFolder->chr, sFolder, qbsFolder->len);

    return qbsFolder;
}

/// @brief Shows the color picker dialog box
/// @param qbsTitle [OPTIONAL] Title of the dialog box
/// @param nDefaultRGB [OPTIONAL] Default selected color
/// @param passed Optional parameter mask
/// @return 0 on cancel (i.e. no color, no alpha, nothing). Else, returns color with alpha set to 255
uint32_t func__guiColorChooserDialog(qbs *qbsTitle, uint32_t nDefaultRGB, int32_t passed) {
    std::string aTitle;

    if (passed & 1)
        aTitle.assign((const char *)qbsTitle->chr, qbsTitle->len);

    if (!(passed & 2))
        nDefaultRGB = 0;

    // Break the color into RGB components
    uint8_t lRGB[3] = {image_get_bgra_red(nDefaultRGB), image_get_bgra_green(nDefaultRGB), image_get_bgra_blue(nDefaultRGB)};

    // On cancel, return 0 (i.e. no color, no alpha, nothing). Else, return color with alpha set to 255
    return !tinyfd_colorChooser(aTitle.c_str(), nullptr, lRGB, lRGB) ? 0 : image_make_bgra(lRGB[0], lRGB[1], lRGB[2], 0xFF);
}

/// @brief Shows the system file open dialog box
/// @param qbsTitle [OPTIONAL] Title of the dialog box
/// @param qbsDefaultPathAndFile [OPTIONAL] The default path (and filename) that will be pre-populated
/// @param qbsFilterPatterns [OPTIONAL] File filters separated using '|' (e.g. "*.png|*.jpg")
/// @param qbsSingleFilterDescription [OPTIONAL] Single filter description (e.g. "Image files")
/// @param nAllowMultipleSelects [OPTIONAL] Should multiple file selection be allowed?
/// @param passed Optional parameter mask
/// @return The file name (or names separated by '|' if multiselect was on) selected by the user or an empty string if the user cancelled
qbs *func__guiOpenFileDialog(qbs *qbsTitle, qbs *qbsDefaultPathAndFile, qbs *qbsFilterPatterns, qbs *qbsSingleFilterDescription, int32_t nAllowMultipleSelects,
                             int32_t passed) {
    std::string aTitle;
    std::string aDefaultPathAndFile;
    std::string aFilterPatterns;
    std::string aSingleFilterDescription;

    if (passed & 1)
        aTitle.assign((const char *)qbsTitle->chr, qbsTitle->len);

    if (passed & 2)
        aDefaultPathAndFile.assign((const char *)qbsDefaultPathAndFile->chr, qbsDefaultPathAndFile->len);

    if (passed & 4)
        aFilterPatterns.assign((const char *)qbsFilterPatterns->chr, qbsFilterPatterns->len);

    const char *sSingleFilterDescription;
    if (passed & 8) {
        aSingleFilterDescription.assign((const char *)qbsSingleFilterDescription->chr, qbsSingleFilterDescription->len);
        sSingleFilterDescription = !qbsSingleFilterDescription->len ? nullptr : aSingleFilterDescription.c_str();
    } else {
        sSingleFilterDescription = nullptr;
    }

    // If nAllowMultipleSelects is < 0 tinyfd_openFileDialog allows a program to force-free any working memory that it
    // may be using and returns NULL. This is really not an issue even if it is not done because tinyfd_openFileDialog
    // 'recycles' it working memory and anything not feed will be taken care of by the OS on program exit. Unfortunately
    // in case of QB64, true is -1. To work around this, we trap any non-zero values and re-interpret those as 1
    nAllowMultipleSelects = !(passed & 16) || !nAllowMultipleSelects ? false : true;

    int32_t aNumOfFilterPatterns;
    auto psaFilterPatterns = gui_tokenize(aFilterPatterns.c_str(), &aNumOfFilterPatterns); // get the number of file filters & count

    auto sFileName = tinyfd_openFileDialog(aTitle.c_str(), aDefaultPathAndFile.c_str(), aNumOfFilterPatterns, psaFilterPatterns, sSingleFilterDescription,
                                           nAllowMultipleSelects);

    gui_free_tokens(psaFilterPatterns); // free memory used by tokenizer

    // Create a new qbs and then copy the string to it
    auto qbsFileName = qbs_new(sFileName ? strlen(sFileName) : 0, 1);
    if (qbsFileName->len)
        memcpy(qbsFileName->chr, sFileName, qbsFileName->len);

    return qbsFileName;
}

/// @brief Shows the system file save dialog box
/// @param qbsTitle [OPTIONAL] Title of the dialog box
/// @param qbsDefaultPathAndFile [OPTIONAL] The default path (and filename) that will be pre-populated
/// @param qbsFilterPatterns [OPTIONAL] File filters separated using '|' (e.g. "*.png|*.jpg")
/// @param qbsSingleFilterDescription [OPTIONAL] Single filter description (e.g. "Image files")
/// @return The file name selected by the user or an empty string if the user cancelled
qbs *func__guiSaveFileDialog(qbs *qbsTitle, qbs *qbsDefaultPathAndFile, qbs *qbsFilterPatterns, qbs *qbsSingleFilterDescription, int32_t passed) {
    std::string aTitle;
    std::string aDefaultPathAndFile;
    std::string aFilterPatterns;
    std::string aSingleFilterDescription;

    if (passed & 1)
        aTitle.assign((const char *)qbsTitle->chr, qbsTitle->len);

    if (passed & 2)
        aDefaultPathAndFile.assign((const char *)qbsDefaultPathAndFile->chr, qbsDefaultPathAndFile->len);

    if (passed & 4)
        aFilterPatterns.assign((const char *)qbsFilterPatterns->chr, qbsFilterPatterns->len);

    const char *sSingleFilterDescription;
    if (passed & 8) {
        aSingleFilterDescription.assign((const char *)qbsSingleFilterDescription->chr, qbsSingleFilterDescription->len);
        sSingleFilterDescription = !qbsSingleFilterDescription->len ? nullptr : aSingleFilterDescription.c_str();
    } else {
        sSingleFilterDescription = nullptr;
    }

    int32_t aNumOfFilterPatterns;
    auto psaFilterPatterns = gui_tokenize(aFilterPatterns.c_str(), &aNumOfFilterPatterns); // get the number of file filters & count

    auto sFileName = tinyfd_saveFileDialog(aTitle.c_str(), aDefaultPathAndFile.c_str(), aNumOfFilterPatterns, psaFilterPatterns, sSingleFilterDescription);

    gui_free_tokens(psaFilterPatterns); // free memory used by tokenizer

    // Create a new qbs and then copy the string to it
    auto qbsFileName = qbs_new(sFileName ? strlen(sFileName) : 0, 1);
    if (qbsFileName->len)
        memcpy(qbsFileName->chr, sFileName, qbsFileName->len);

    return qbsFileName;
}

/// @brief This is used internally by libqb to show warning and failure messages
/// @param message The message the will show inside the dialog box
/// @param title The dialog box title
/// @param type The type of dialog box (see tinyfd_messageBox)
/// @return returns the value returned by tinyfd_messageBox
int gui_alert(const char *message, const char *title, const char *type) { return tinyfd_messageBox(title, message, type, "error", 1); }

/// @brief This is used internally by libqb to show warning and failure messages
/// @param fmt A string that contains a printf style format
/// @param ... Additional arguments
/// @return true if successful, false otherwise
bool gui_alert(const char *fmt, ...) {
    if (!fmt)
        return false;

    size_t l = strlen(fmt) * 2 + UCHAR_MAX;

    char *buf = (char *)malloc(l);
    if (!buf)
        return false;

    va_list args;
    va_start(args, fmt);

    if (vsnprintf(buf, l, fmt, args) < 0) {
        va_end(args);
        free(buf);

        return false;
    }

    va_end(args);

    gui_alert(buf, "Alert", "ok");

    free(buf);

    return true;
}
