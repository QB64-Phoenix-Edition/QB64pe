//----------------------------------------------------------------------------------------------------------------------
// foo_midi: A foobar2000 component to play MIDI files (https://github.com/stuerp/foo_midi)
// Copyright (c) 2022-2024 Peter Stuer
//
// Modified by a740g for QB64-PE
//----------------------------------------------------------------------------------------------------------------------

#include "VSTiPlayer.h"

VSTiPlayer::VSTiPlayer(InstrumentBankManager *ibm) : MIDIPlayer(), instrumentBankManager(ibm) {
    _IsCOMInitialized = false;
    _IsTerminating = false;

    _hReadEvent = NULL;
    _hPipeInRead = NULL;
    _hPipeInWrite = NULL;
    _hPipeOutRead = NULL;
    _hPipeOutWrite = NULL;
    _hProcess = NULL;
    _hThread = NULL;

    _ChannelCount = 0;

    _ProcessorArchitecture = 0;
    _UniqueId = 0;
    _VendorVersion = 0;
}

VSTiPlayer::~VSTiPlayer() {
    Shutdown();
}

void VSTiPlayer::GetChunk(std::vector<uint8_t> &chunk) {
    WriteBytes(static_cast<uint32_t>(VSTHostCommand::GetChunk));

    const uint32_t Code = ReadCode();

    if (Code != 0) {
        Shutdown();
        return;
    }

    const uint32_t Size = ReadCode();

    chunk.resize(Size);

    if (Size != 0)
        ReadBytes(chunk.data(), Size);
}

void VSTiPlayer::SetChunk(const void *data, size_t size) {
    if ((_Chunk.size() == 0) || ((_Chunk.size() == size) && (size != 0) && (data != (const void *)_Chunk.data()))) {
        _Chunk.resize(size);

        if (size != 0)
            ::memcpy(_Chunk.data(), data, size);
    }

    WriteBytes(static_cast<uint32_t>(VSTHostCommand::SetChunk));
    WriteBytes((uint32_t)size);
    WriteBytesOverlapped(data, (uint32_t)size);

    const uint32_t Code = ReadCode();

    if (Code != 0)
        Shutdown();
}

bool VSTiPlayer::HasEditor() {
    WriteBytes(static_cast<uint32_t>(VSTHostCommand::HasEditor));

    uint32_t Code = ReadCode();

    if (Code != 0) {
        Shutdown();
        return false;
    }

    Code = ReadCode();

    return Code != 0;
}

void VSTiPlayer::DisplayEditorModal() {
    WriteBytes(static_cast<uint32_t>(VSTHostCommand::DisplayEditorModal));

    const uint32_t Code = ReadCode();

    if (Code != 0)
        Shutdown();
}

static char __PrintHexDigit(unsigned val) {
    static constexpr char table[16] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};
    return table[val & 0xF];
}

static void __PrintHex(unsigned val, char *&out, unsigned bytes) {
    unsigned n;
    for (n = 0; n < bytes; n++) {
        unsigned char c = (unsigned char)((val >> ((bytes - 1 - n) << 3)) & 0xFF);
        *(out++) = __PrintHexDigit(c >> 4);
        *(out++) = __PrintHexDigit(c & 0xF);
    }
    *out = 0;
}

static std::string __PrintGUID(const GUID &p_guid) {
    char data[64];
    char *out = data;
    __PrintHex(p_guid.Data1, out, 4);
    *(out++) = '-';
    __PrintHex(p_guid.Data2, out, 2);
    *(out++) = '-';
    __PrintHex(p_guid.Data3, out, 2);
    *(out++) = '-';
    __PrintHex(p_guid.Data4[0], out, 1);
    __PrintHex(p_guid.Data4[1], out, 1);
    *(out++) = '-';
    __PrintHex(p_guid.Data4[2], out, 1);
    __PrintHex(p_guid.Data4[3], out, 1);
    __PrintHex(p_guid.Data4[4], out, 1);
    __PrintHex(p_guid.Data4[5], out, 1);
    __PrintHex(p_guid.Data4[6], out, 1);
    __PrintHex(p_guid.Data4[7], out, 1);
    *out = 0;
    return data;
}

static bool __CreatePipeName(std::string &pipeName) {
    GUID guid;

    if (FAILED(::CoCreateGuid(&guid)))
        return false;

    pipeName = "\\\\.\\pipe\\";
    pipeName += __PrintGUID(guid);

    return true;
}

bool VSTiPlayer::Startup() {
    if (_IsInitialized)
        return true;

    if (!instrumentBankManager || instrumentBankManager->GetType() != InstrumentBankManager::Type::VSTi ||
        instrumentBankManager->GetLocation() == InstrumentBankManager::Location::Memory) {
        return false;
    }

    _ProcessorArchitecture = GetProcessorArchitecture(instrumentBankManager->GetPath());

    if (_ProcessorArchitecture == 0)
        return false;

    if (!_IsCOMInitialized) {
        switch (::CoInitialize(NULL)) {
        case S_OK:
            // We initialized COM on this thread first
            // So, we will set the _IsCOMInitialized flag
            _IsCOMInitialized = true;
            break;

        case RPC_E_CHANGED_MODE:
        case S_FALSE:
            // Something else already initialized COM on this thread
            // So, we will not set the _IsCOMInitialized flag
            break;

        default:
            return false;
        }
    }

    _hReadEvent = ::CreateEvent(NULL, TRUE, FALSE, NULL);

    SECURITY_ATTRIBUTES sa = {
        sizeof(sa),
        nullptr,
        TRUE,
    };

    std::string InPipeName, OutPipeName;

    {
        if (!__CreatePipeName(InPipeName) || !__CreatePipeName(OutPipeName)) {
            Shutdown();

            return false;
        }
    }

    {
        HANDLE hPipe = ::CreateNamedPipeA(InPipeName.c_str(), PIPE_ACCESS_OUTBOUND | FILE_FLAG_FIRST_PIPE_INSTANCE | FILE_FLAG_OVERLAPPED, PIPE_TYPE_BYTE, 1,
                                          65536, 65536, 0, &sa);

        if (hPipe == INVALID_HANDLE_VALUE) {
            Shutdown();

            return false;
        }

        _hPipeInRead = ::CreateFileA(InPipeName.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, &sa, OPEN_EXISTING, 0, NULL);

        ::DuplicateHandle(::GetCurrentProcess(), hPipe, ::GetCurrentProcess(), &_hPipeInWrite, 0, FALSE, DUPLICATE_SAME_ACCESS);

        ::CloseHandle(hPipe);
    }

    {
        HANDLE hPipe = ::CreateNamedPipeA(OutPipeName.c_str(), PIPE_ACCESS_INBOUND | FILE_FLAG_FIRST_PIPE_INSTANCE | FILE_FLAG_OVERLAPPED, PIPE_TYPE_BYTE, 1,
                                          65536, 65536, 0, &sa);

        if (hPipe == INVALID_HANDLE_VALUE) {
            Shutdown();

            return false;
        }

        _hPipeOutWrite = ::CreateFileA(OutPipeName.c_str(), GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, &sa, OPEN_EXISTING, 0, NULL);

        ::DuplicateHandle(::GetCurrentProcess(), hPipe, ::GetCurrentProcess(), &_hPipeOutRead, 0, FALSE, DUPLICATE_SAME_ACCESS);

        ::CloseHandle(hPipe);
    }

    std::string CommandLine = "\"";

    {
        std::string fPath, fName;
        filepath_split(instrumentBankManager->GetPath(), fPath, fName);

        CommandLine += fPath;

        CommandLine += (_ProcessorArchitecture == 64) ? "vsthost64.exe" : "vsthost32.exe";
        CommandLine += "\" \"";
        CommandLine += instrumentBankManager->GetPath();
        CommandLine += "\" ";

        {
            uint32_t Sum = 0;

            auto ch = instrumentBankManager->GetPath();

            while (*ch) {
                Sum += *ch++ * 820109;
            }

            std::stringstream sumHex;
            sumHex << std::hex << Sum;

            CommandLine += sumHex.str();
        }
    }

    {
        STARTUPINFOA si = {};

        si.cb = sizeof(si);
        si.hStdInput = _hPipeInRead;
        si.hStdOutput = _hPipeOutWrite;
        si.hStdError = ::GetStdHandle(STD_ERROR_HANDLE);
        //  si.wShowWindow = SW_HIDE;
        si.dwFlags |= STARTF_USESTDHANDLES; // | STARTF_USESHOWWINDOW;

        PROCESS_INFORMATION pi = {};

        std::vector<CHAR> cmdLineBuffer(CommandLine.size() + 1, 0);
        std::copy(CommandLine.begin(), CommandLine.end(), cmdLineBuffer.begin());

        if (!::CreateProcessA(NULL, cmdLineBuffer.data(), NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi)) {
            Shutdown();

            return false;
        }

        // Close remote handles so the pipes will break when the process terminates.
        ::CloseHandle(_hPipeOutWrite);
        _hPipeOutWrite = 0;

        ::CloseHandle(_hPipeInRead);
        _hPipeInRead = 0;

        _hProcess = pi.hProcess;
        _hThread = pi.hThread;

        ::SetPriorityClass(_hProcess, ::GetPriorityClass(::GetCurrentProcess()));
        ::SetThreadPriority(_hThread, ::GetThreadPriority(::GetCurrentThread()));
    }

    // Get the startup information.
    const uint32_t Code = ReadCode();

    if (Code != 0) {
        Shutdown();

        return false;
    }

    {
        uint32_t NameLength = ReadCode();
        uint32_t VendorNameLength = ReadCode();
        uint32_t ProductNameLength = ReadCode();

        _VendorVersion = ReadCode();
        _UniqueId = ReadCode();
        _ChannelCount = ReadCode();

        {
            // VST always uses float samples.
            _Samples.clear();
            _Samples.resize(renderFrames * _ChannelCount);

            _Name.resize(NameLength);
            ReadBytes(&_Name[0], NameLength);

            _VendorName.resize(VendorNameLength);
            ReadBytes(&_VendorName[0], VendorNameLength);

            _ProductName.resize(ProductNameLength);
            ReadBytes(&_ProductName[0], ProductNameLength);
        }
    }

    if (_Chunk.size() != 0)
        SetChunk(_Chunk.data(), _Chunk.size());

    WriteBytes(static_cast<uint32_t>(VSTHostCommand::SetSampleRate));
    WriteBytes(sizeof(uint32_t));
    WriteBytes(_SampleRate);

    const uint32_t code = ReadCode();

    if (code != 0)
        Shutdown();

    _IsInitialized = IsHostRunning();

    Configure(MIDIFlavor::None, false);

    return _IsInitialized;
}

void VSTiPlayer::Shutdown() {
    if (_IsTerminating)
        return;

    _IsTerminating = true;

    if (_hProcess) {
        WriteBytes(static_cast<uint32_t>(VSTHostCommand::Exit));

        ::WaitForSingleObject(_hProcess, 5000);
        ::TerminateProcess(_hProcess, 0);

        ::CloseHandle(_hThread);
        _hThread = NULL;

        ::CloseHandle(_hProcess);
        _hProcess = NULL;
    }

    if (_hPipeInRead) {
        ::CloseHandle(_hPipeInRead);
        _hPipeInRead = NULL;
    }

    if (_hPipeInWrite) {
        ::CloseHandle(_hPipeInWrite);
        _hPipeInWrite = NULL;
    }

    if (_hPipeOutRead) {
        ::CloseHandle(_hPipeOutRead);
        _hPipeOutRead = NULL;
    }

    if (_hPipeOutWrite) {
        ::CloseHandle(_hPipeOutWrite);
        _hPipeOutWrite = NULL;
    }

    if (_hReadEvent) {
        ::CloseHandle(_hReadEvent);
        _hReadEvent = 0;
    }

    if (_IsCOMInitialized) {
        ::CoUninitialize();
        _IsCOMInitialized = false;
    }

    _IsTerminating = false;

    _IsInitialized = false;
}

void VSTiPlayer::Render(audio_sample *sampleData, uint32_t sampleCount) {
    WriteBytes(static_cast<uint32_t>(VSTHostCommand::RenderSamples));
    WriteBytes(sampleCount);

    if (ReadCode() != 0) {
        Shutdown();
        ::memset(sampleData, 0, size_t(sampleCount) * sizeof(audio_sample) << 1); // clear buffer for stereo channels only!

        return;
    }

    if (!_Samples.size()) {
        return;
    }

    while (sampleCount != 0) {
        auto ToDo = std::min(sampleCount, renderFrames);

        ReadBytes(&_Samples[0], uint32_t(ToDo * _ChannelCount * sizeof(float))); // read whatever multichannel data we have

        // Convert the format of the rendered output.
        for (uint32_t i = 0; i < ToDo; ++i) {
            // a740g's rant. Feel free to ignore.
            // Down-mix to stereo. This is sad and probably not the right way to down-mix, but ok.
            // I have not seen a VSTi that does more than 2 channels but then my VSTi knowledge is limited.
            // This plugin was specifically created for the Yamaha S-YXG50 VSTi (https://veg.by/en/projects/syxg50/) and it works well with that.
            // Even the Roland Sound Canvas VSTi works like a champ. Honestly, I do not care about anything else.
            // So, this will stay this way until someone starts nagging. xD
            float left = 0.0f;
            float right = 0.0f;

            for (uint32_t j = 0; j < _ChannelCount; ++j) {
                if (j & 1u)
                    right += _Samples[i * _ChannelCount + j]; // odd channels go to the right
                else
                    left += _Samples[i * _ChannelCount + j]; // even channels go to the left
            }

            sampleData[i << 1] = left;        // left channel
            sampleData[(i << 1) + 1] = right; // right channel
        }

        sampleData += ToDo << 1;
        sampleCount -= ToDo;
    }
}

void VSTiPlayer::SendEvent(uint32_t b) {
    WriteBytes(static_cast<uint32_t>(VSTHostCommand::SendMIDIEvent));
    WriteBytes(b);

    const uint32_t code = ReadCode();

    if (code != 0)
        Shutdown();
}

void VSTiPlayer::SendSysEx(const uint8_t *data, size_t size, uint32_t portNumber) {
    const uint32_t SizeAndPort = ((uint32_t)size & 0xFFFFFF) | (portNumber << 24);

    WriteBytes(static_cast<uint32_t>(VSTHostCommand::SendSysexEvent));
    WriteBytes(SizeAndPort);
    WriteBytesOverlapped(data, (uint32_t)size);

    const uint32_t code = ReadCode();

    if (code != 0)
        Shutdown();
}

void VSTiPlayer::SendEvent(uint32_t data, uint32_t time) {
    WriteBytes(static_cast<uint32_t>(VSTHostCommand::SendMIDIEventWithTimestamp));
    WriteBytes(data);
    WriteBytes(time);

    const uint32_t code = ReadCode();

    if (code != 0)
        Shutdown();
}

void VSTiPlayer::SendSysEx(const uint8_t *data, size_t size, uint32_t portNumber, uint32_t time) {
    const uint32_t SizeAndPort = ((uint32_t)size & 0xFFFFFF) | (portNumber << 24);

    WriteBytes(static_cast<uint32_t>(VSTHostCommand::SendSysexEventWithTimestamp));
    WriteBytes(SizeAndPort);
    WriteBytes(time);
    WriteBytesOverlapped(data, (uint32_t)size);

    const uint32_t code = ReadCode();

    if (code != 0)
        Shutdown();
}

bool VSTiPlayer::IsHostRunning() noexcept {
    if (_hProcess && ::WaitForSingleObject(_hProcess, 0) == WAIT_TIMEOUT)
        return true;

    return false;
}

uint32_t VSTiPlayer::ReadCode() noexcept {
    uint32_t Code;

    ReadBytes(&Code, sizeof(Code));

    return Code;
}

void VSTiPlayer::ReadBytes(void *data, uint32_t size) noexcept {
    if (size == 0)
        return;

    if (IsHostRunning()) {
        uint8_t *Data = (uint8_t *)data;
        uint32_t BytesTotal = 0;

        while (BytesTotal < size) {
            const uint32_t BytesRead = ReadBytesOverlapped(Data + BytesTotal, size - BytesTotal);

            if (BytesRead == 0) {
                ::memset(data, 0xFF, size);
                break;
            }

            BytesTotal += BytesRead;
        }
    } else {
        ::memset(data, 0xFF, size);
    }
}

uint32_t VSTiPlayer::ReadBytesOverlapped(void *data, uint32_t size) noexcept {
    ::ResetEvent(_hReadEvent);

    ::SetLastError(NO_ERROR);

    DWORD BytesRead;
    OVERLAPPED ol = {};

    ol.hEvent = _hReadEvent;

    if (::ReadFile(_hPipeOutRead, data, size, &BytesRead, &ol))
        return BytesRead;

    if (::GetLastError() != ERROR_IO_PENDING) {
        return 0;
    }

    const HANDLE handles[1] = {_hReadEvent};

    ::SetLastError(NO_ERROR);

    DWORD state = ::WaitForMultipleObjects(_countof(handles), &handles[0], FALSE, INFINITE);

    if (state == WAIT_OBJECT_0 && ::GetOverlappedResult(_hPipeOutRead, &ol, &BytesRead, TRUE))
        return BytesRead;

    ::CancelIoEx(_hPipeOutRead, &ol);

    return 0;
}

void VSTiPlayer::WriteBytes(uint32_t code) noexcept {
    WriteBytesOverlapped(&code, sizeof(code));
}

void VSTiPlayer::WriteBytesOverlapped(const void *data, uint32_t size) noexcept {
    if ((size == 0) || !IsHostRunning())
        return;

    DWORD BytesWritten;

    if (!::WriteFile(_hPipeInWrite, data, size, &BytesWritten, nullptr) || (BytesWritten < size))
        Shutdown();
}
