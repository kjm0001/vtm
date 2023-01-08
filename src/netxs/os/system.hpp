// Copyright (c) NetXS Group.
// Licensed under the MIT license.

#pragma once

#if (defined(__unix__) || defined(__APPLE__)) && !defined(__linux__)
    #define __BSD__
#endif

#if defined(__clang__) || defined(__APPLE__)
    #pragma clang diagnostic ignored "-Wunused-variable"
    #pragma clang diagnostic ignored "-Wunused-function"
#endif

#include "file_system.hpp"
#include "../text/logger.hpp"
#include "../console/input.hpp"
#include "../abstract/ptr.hpp"
#include "../console/richtext.hpp"
#include "../ui/layout.hpp"

#include <type_traits>
#include <iostream>         // std::cout

#if defined(_WIN32)

    #ifndef NOMINMAX
        #define NOMINMAX
    #endif

    #pragma warning(disable:4996) // disable std::getenv warnimg
    #pragma comment(lib, "Advapi32.lib")  // GetUserName()

    #include <Windows.h>
    #include <Psapi.h>      // GetModuleFileNameEx
    #include <Wtsapi32.h>   // WTSEnumerateSessions, get_logged_usres
    #include <Shlwapi.h>
    #include <shobjidl.h>   // IShellLink
    #include <shlguid.h>    // IID_IShellLink
    #include <Shlobj.h>     // create_shortcut: SHGetFolderLocation / (SHGetFolderPathW - vist and later) CLSID_ShellLink
    #include <Psapi.h>      // GetModuleFileNameEx

    #include <DsGetDC.h>    // DsGetDcName
    #include <LMCons.h>     // DsGetDcName
    #include <Lmapibuf.h>   // DsGetDcName

    #include <Sddl.h>       // security_descriptor

    #include <winternl.h>   // NtOpenFile
    #include <wingdi.h>     // TranslateCharsetInfo

#else

    #include <errno.h>      // ::errno
    #include <spawn.h>      // ::exec
    #include <unistd.h>     // ::gethostname(), ::getpid(), ::read()
    #include <sys/param.h>  //
    #include <sys/types.h>  // ::getaddrinfo
    #include <sys/socket.h> // ::shutdown() ::socket(2)
    #include <netdb.h>      //

    #include <stdio.h>
    #include <unistd.h>     // ::read(), PIPE_BUF
    #include <sys/un.h>
    #include <stdlib.h>

    #include <csignal>      // ::signal()
    #include <termios.h>    // console raw mode
    #include <sys/ioctl.h>  // ::ioctl
    #include <sys/wait.h>   // ::waitpid
    #include <syslog.h>     // syslog, daemonize

    #include <sys/types.h>
    #include <sys/stat.h>
    #include <fcntl.h>      // ::splice()

    #if defined(__linux__)
        #include <sys/vt.h> // ::console_ioctl()
        #if defined(__ANDROID__)
            #include <linux/kd.h>   // ::console_ioctl()
        #else
            #include <sys/kd.h>     // ::console_ioctl()
        #endif
        #include <linux/keyboard.h> // ::keyb_ioctl()
    #endif

    #if defined(__APPLE__)
        #include <mach-o/dyld.h>    // ::_NSGetExecutablePath()
    #elif defined(__BSD__)
        #include <sys/types.h>  // ::sysctl()
        #include <sys/sysctl.h>
    #endif

    extern char **environ;

#endif

namespace netxs::os
{
    namespace ipc
    {
        struct stdcon;
    }

    namespace fs = std::filesystem;
    using namespace std::chrono_literals;
    using namespace std::literals;
    using namespace netxs::ui::atoms;
    using list = std::vector<text>;
    using xipc = std::shared_ptr<ipc::stdcon>;
    using page = console::page;
    using para = console::para;
    using rich = console::rich;

    enum role { client, server };

    static constexpr auto stdin_buf_size = si32{ 1024 };
    static auto is_daemon = faux;

    #if defined(_WIN32)

        using sigA = BOOL;
        using sigB = DWORD;
        using pid_t = DWORD;
        using fd_t = HANDLE;
        using conmode = DWORD[2];
        static const auto INVALID_FD   = fd_t{ INVALID_HANDLE_VALUE              };
        static const auto STDIN_FD     = fd_t{ ::GetStdHandle(STD_INPUT_HANDLE)  };
        static const auto STDOUT_FD    = fd_t{ ::GetStdHandle(STD_OUTPUT_HANDLE) };
        static const auto STDERR_FD    = fd_t{ ::GetStdHandle(STD_ERROR_HANDLE)  };
        static const auto PIPE_BUF     = si32{ 65536 };
        static const auto WR_PIPE_PATH = "\\\\.\\pipe\\w_";
        static const auto RD_PIPE_PATH = "\\\\.\\pipe\\r_";
        static auto clipboard_sequence = std::numeric_limits<DWORD>::max();
        static auto clipboard_mutex    = std::mutex();
        static auto cf_text = CF_UNICODETEXT;
        static auto cf_utf8 = CF_TEXT;
        static auto cf_rich = ::RegisterClipboardFormatA("Rich Text Format");
        static auto cf_html = ::RegisterClipboardFormatA("HTML Format");
        static auto cf_ansi = ::RegisterClipboardFormatA("ANSI/VT Format");
        static auto cf_sec1 = ::RegisterClipboardFormatA("ExcludeClipboardContentFromMonitorProcessing");
        static auto cf_sec2 = ::RegisterClipboardFormatA("CanIncludeInClipboardHistory");
        static auto cf_sec3 = ::RegisterClipboardFormatA("CanUploadToCloudClipboard");

        //static constexpr auto security_descriptor_string =
        //	//"D:P(A;NP;GA;;;SY)(A;NP;GA;;;BA)(A;NP;GA;;;WD)";
        //	"O:AND:P(A;NP;GA;;;SY)(A;NP;GA;;;BA)(A;NP;GA;;;CO)";
        //	//"D:P(A;NP;GA;;;SY)(A;NP;GA;;;BA)(A;NP;GA;;;AU)";// Authenticated users
        //	//"D:P(A;NP;GA;;;SY)(A;NP;GA;;;BA)(A;NP;GA;;;CO)"; // CREATOR OWNER
        //	//"D:P(A;OICI;GA;;;SY)(A;OICI;GA;;;BA)(A;OICI;GA;;;CO)";
        //	//  "D:"  DACL
        //	//  "P"   SDDL_PROTECTED        The SE_DACL_PROTECTED flag is set.
        //	//  "A"   SDDL_ACCESS_ALLOWED
        //	// ace_flags:
        //	//  "OI"  SDDL_OBJECT_INHERIT
        //	//  "CI"  SDDL_CONTAINER_INHERIT
        //	//  "NP"  SDDL_NO_PROPAGATE
        //	// rights:
        //	//  "GA"  SDDL_GENERIC_ALL
        //	// account_sid: see https://docs.microsoft.com/en-us/windows/win32/secauthz/sid-strings
        //	//  "SY"  SDDL_LOCAL_SYSTEM
        //	//  "BA"  SDDL_BUILTIN_ADMINISTRATORS
        //	//  "CO"  SDDL_CREATOR_OWNER
        //	//  "WD"  SDDL_EVERYONE

    #else

        using sigA = void;
        using sigB = int;
        using fd_t = int;
        using conmode = ::termios;
        static constexpr auto INVALID_FD = fd_t{ -1            };
        static constexpr auto STDIN_FD   = fd_t{ STDIN_FILENO  };
        static constexpr auto STDOUT_FD  = fd_t{ STDOUT_FILENO };
        static constexpr auto STDERR_FD  = fd_t{ STDERR_FILENO };

        void fdcleanup() // Close all file descriptors except the standard ones.
        {
            auto maxfd = ::sysconf(_SC_OPEN_MAX);
            auto minfd = std::max({ STDIN_FD, STDOUT_FD, STDERR_FD });
            while (++minfd < maxfd)
            {
                ::close(minfd);
            }
        }

    #endif

    class args
    {
        using list = std::list<text>;
        using it_t = list::iterator;

        list data{};
        it_t iter{};

        // args: Recursive argument matching.
        template<class I>
        auto test(I&& item) { return faux; }
        // args: Recursive argument matching.
        template<class I, class T, class ...Args>
        auto test(I&& item, T&& sample, Args&&... args)
        {
            return item == sample || test(item, std::forward<Args>(args)...);
        }

    public:
        args(int argc, char** argv)
        {
            auto head = argv + 1;
            auto tail = argv + argc;
            while (head != tail)
            {
                data.splice(data.end(), split(*head++));
            }
            reset();
        }
        // args: Split command line options into tokens.
        template<class T = list>
        static T split(view line)
        {
            auto args = T{};
            line = utf::trim(line);
            while (line.size())
            {
                auto item = utf::get_token(line);
                if (item.size()) args.emplace_back(item);
            }
            return args;
        }
        // args: Reset arg iterator to begin.
        void reset()
        {
            iter = data.begin();
        }
        // args: Return true if not the end.
        operator bool () const { return iter != data.end(); }
        // args: Test the current argument and step forward if met.
        template<class ...Args>
        auto match(Args&&... args)
        {
            auto result = iter != data.end() && test(*iter, std::forward<Args>(args)...);
            if (result) ++iter;
            return result;
        }
        // args: Return current argument and step forward.
        template<class ...Args>
        auto next()
        {
            return iter != data.end() ? view{ *iter++ }
                                      : view{};
        }
        // args: Return the rest of the command line arguments.
        auto rest()
        {
            auto crop = text{};
            if (iter != data.end())
            {
                crop += *iter++;
                while (iter != data.end())
                {
                    crop.push_back(' ');
                    crop += *iter++;
                }
            }
            return crop;
        }
    };

    struct nothing
    {
        template<class T>
        operator T () { return T{}; }
    };

    void close(fd_t& h)
    {
        if (h != INVALID_FD)
        {
            #if defined(_WIN32)
                ::CloseHandle(h);
            #else
                ::close(h);
            #endif
            h = INVALID_FD;
        }
    }
    auto error()
    {
        #if defined(_WIN32)
            return ::GetLastError();
        #else
            return errno;
        #endif
    }
    template<class ...Args>
    auto fail(Args&&... msg)
    {
        log("  os: ", ansi::fgc(tint::redlt), msg..., " (", os::error(), ") ", ansi::nil());
        return nothing{};
    };
    template<class T>
    auto ok(T error_condition, text msg = {})
    {
        if (
            #if defined(_WIN32)
                error_condition == 0
            #else
                error_condition == (T)-1
            #endif
        )
        {
            os::fail(msg);
            return faux;
        }
        else return true;
    }

    #if defined(_WIN32)

        template<class T1, class T2 = si32>
        auto kbstate(si32& modstate, T1 ms_ctrls, T2 scancode = {}, bool pressed = {})
        {
            if (scancode == 0x2a)
            {
                if (pressed) modstate |= input::hids::LShift;
                else         modstate &=~input::hids::LShift;
            }
            if (scancode == 0x36)
            {
                if (pressed) modstate |= input::hids::RShift;
                else         modstate &=~input::hids::RShift;
            }
            auto lshift = modstate & input::hids::LShift;
            auto rshift = modstate & input::hids::RShift;
            bool lalt   = ms_ctrls & LEFT_ALT_PRESSED;
            bool ralt   = ms_ctrls & RIGHT_ALT_PRESSED;
            bool lctrl  = ms_ctrls & LEFT_CTRL_PRESSED;
            bool rctrl  = ms_ctrls & RIGHT_CTRL_PRESSED;
            bool nums   = ms_ctrls & NUMLOCK_ON;
            bool caps   = ms_ctrls & CAPSLOCK_ON;
            bool scrl   = ms_ctrls & SCROLLLOCK_ON;
            auto state  = ui32{};
            if (lshift) state |= input::hids::LShift;
            if (rshift) state |= input::hids::RShift;
            if (lalt  ) state |= input::hids::LAlt;
            if (ralt  ) state |= input::hids::RAlt;
            if (lctrl ) state |= input::hids::LCtrl;
            if (rctrl ) state |= input::hids::RCtrl;
            if (nums  ) state |= input::hids::NumLock;
            if (caps  ) state |= input::hids::CapsLock;
            if (scrl  ) state |= input::hids::ScrlLock;
            return state;
        }
        template<class T1>
        auto ms_kbstate(T1 ctrls)
        {
            bool lshift = ctrls & input::hids::LShift;
            bool rshift = ctrls & input::hids::RShift;
            bool lalt   = ctrls & input::hids::LAlt;
            bool ralt   = ctrls & input::hids::RAlt;
            bool lctrl  = ctrls & input::hids::LCtrl;
            bool rctrl  = ctrls & input::hids::RCtrl;
            bool nums   = ctrls & input::hids::NumLock;
            bool caps   = ctrls & input::hids::CapsLock;
            bool scrl   = ctrls & input::hids::ScrlLock;
            auto state  = ui32{};
            if (lshift) state |= SHIFT_PRESSED;
            if (rshift) state |= SHIFT_PRESSED;
            if (lalt  ) state |= LEFT_ALT_PRESSED;
            if (ralt  ) state |= RIGHT_ALT_PRESSED;
            if (lctrl ) state |= LEFT_CTRL_PRESSED;
            if (rctrl ) state |= RIGHT_CTRL_PRESSED;
            if (nums  ) state |= NUMLOCK_ON;
            if (caps  ) state |= CAPSLOCK_ON;
            if (scrl  ) state |= SCROLLLOCK_ON;
            return state;
        }

        class nt
        {
            using NtOpenFile_ptr = std::decay<decltype(::NtOpenFile)>::type;
            using ConsoleCtl_ptr = NTSTATUS(*)(ui32, void*, ui32);

            HMODULE         ntdll_dll{};
            HMODULE        user32_dll{};
            NtOpenFile_ptr NtOpenFile{};
            ConsoleCtl_ptr ConsoleCtl{};

            nt()
            {
                ntdll_dll  = ::LoadLibraryEx("ntdll.dll",  nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
                user32_dll = ::LoadLibraryEx("user32.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
                if (!ntdll_dll || !user32_dll) os::fail("LoadLibraryEx(ntdll.dll | user32.dll)");
                else
                {
                    NtOpenFile = reinterpret_cast<NtOpenFile_ptr>(::GetProcAddress( ntdll_dll, "NtOpenFile"));
                    ConsoleCtl = reinterpret_cast<ConsoleCtl_ptr>(::GetProcAddress(user32_dll, "ConsoleControl"));
                    if (!NtOpenFile) os::fail("::GetProcAddress(NtOpenFile)");
                    if (!ConsoleCtl) os::fail("::GetProcAddress(ConsoleControl)");
                }
            }

            void operator=(nt const&) = delete;
            nt(nt const&)             = delete;
            nt(nt&& other)
                : ntdll_dll{ other.ntdll_dll  },
                 user32_dll{ other.user32_dll },
                 NtOpenFile{ other.NtOpenFile },
                 ConsoleCtl{ other.ConsoleCtl }
            {
                other.ntdll_dll  = {};
                other.user32_dll = {};
                other.NtOpenFile = {};
                other.ConsoleCtl = {};
            }

            static auto& get_ntdll()
            {
                static auto ref = nt{};
                return ref;
            }

        public:
           ~nt()
            {
                if (ntdll_dll ) ::FreeLibrary(ntdll_dll );
                if (user32_dll) ::FreeLibrary(user32_dll);
            }

            constexpr explicit operator bool() const { return NtOpenFile != nullptr; }

            struct status
            {
                static constexpr auto success               = (NTSTATUS)0x00000000L;
                static constexpr auto unsuccessful          = (NTSTATUS)0xC0000001L;
                static constexpr auto illegal_function      = (NTSTATUS)0xC00000AFL;
                static constexpr auto not_found             = (NTSTATUS)0xC0000225L;
                static constexpr auto not_supported         = (NTSTATUS)0xC00000BBL;
                static constexpr auto buffer_too_small      = (NTSTATUS)0xC0000023L;
                static constexpr auto invalid_buffer_size   = (NTSTATUS)0xC0000206L;
                static constexpr auto invalid_handle        = (NTSTATUS)0xC0000008L;
                static constexpr auto control_c_exit        = (NTSTATUS)0xC000013AL;
            };

            template<class ...Args>
            static auto OpenFile(Args... args)
            {
                auto& inst = get_ntdll();
                return inst ? inst.NtOpenFile(std::forward<Args>(args)...)
                            : nt::status::not_found;
            }
            template<class ...Args>
            static auto UserConsoleControl(Args... args)
            {
                auto& inst = get_ntdll();
                return inst ? inst.ConsoleCtl(std::forward<Args>(args)...)
                            : nt::status::not_found;
            }
            template<class I = noop, class O = noop>
            static auto ioctl(DWORD dwIoControlCode, fd_t hDevice, I&& send = {}, O&& recv = {}) -> NTSTATUS
            {
                auto BytesReturned   = DWORD{};
                auto lpInBuffer      = std::is_same_v<std::decay_t<I>, noop> ? nullptr : static_cast<void*>(&send);
                auto nInBufferSize   = std::is_same_v<std::decay_t<I>, noop> ? 0       : static_cast<DWORD>(sizeof(send));
                auto lpOutBuffer     = std::is_same_v<std::decay_t<O>, noop> ? nullptr : static_cast<void*>(&recv);
                auto nOutBufferSize  = std::is_same_v<std::decay_t<O>, noop> ? 0       : static_cast<DWORD>(sizeof(recv));
                auto lpBytesReturned = &BytesReturned;
                auto ok = ::DeviceIoControl(hDevice,
                                            dwIoControlCode,
                                            lpInBuffer,
                                            nInBufferSize,
                                            lpOutBuffer,
                                            nOutBufferSize,
                                            lpBytesReturned,
                                            nullptr);
                return ok ? ERROR_SUCCESS
                          : os::error();
            }
            static auto object(view        path,
                               ACCESS_MASK mask,
                               ULONG       flag,
                               ULONG       opts = {},
                               HANDLE      boss = {})
            {
                auto hndl = INVALID_FD;
                auto wtxt = utf::to_utf(path);
                auto size = wtxt.size() * sizeof(wtxt[0]);
                auto attr = OBJECT_ATTRIBUTES{};
                auto stat = IO_STATUS_BLOCK{};
                auto name = UNICODE_STRING{
                    .Length        = (decltype(UNICODE_STRING::Length)       )(size),
                    .MaximumLength = (decltype(UNICODE_STRING::MaximumLength))(size + sizeof(wtxt[0])),
                    .Buffer        = wtxt.data(),
                };
                InitializeObjectAttributes(&attr, &name, flag, boss, nullptr);
                auto status = nt::OpenFile(&hndl, mask, &attr, &stat, FILE_SHARE_READ
                                                                    | FILE_SHARE_WRITE
                                                                    | FILE_SHARE_DELETE, opts);
                if (status != nt::status::success)
                {
                    log("  os: unexpected result when access system object '", path, "', ntstatus ", status);
                    return INVALID_FD;
                }
                else return hndl;
            }

            struct console
            {
                enum fx
                {
                    undef,
                    connect,
                    disconnect,
                    create,
                    close,
                    write,
                    read,
                    subfx,
                    flush,
                    count,
                };
                struct op
                {
                    static constexpr auto read_io                 = CTL_CODE(FILE_DEVICE_CONSOLE, 1,  METHOD_OUT_DIRECT,  FILE_ANY_ACCESS);
                    static constexpr auto complete_io             = CTL_CODE(FILE_DEVICE_CONSOLE, 2,  METHOD_NEITHER,     FILE_ANY_ACCESS);
                    static constexpr auto read_input              = CTL_CODE(FILE_DEVICE_CONSOLE, 3,  METHOD_NEITHER,     FILE_ANY_ACCESS);
                    static constexpr auto write_output            = CTL_CODE(FILE_DEVICE_CONSOLE, 4,  METHOD_NEITHER,     FILE_ANY_ACCESS);
                    static constexpr auto issue_user_io           = CTL_CODE(FILE_DEVICE_CONSOLE, 5,  METHOD_OUT_DIRECT,  FILE_ANY_ACCESS);
                    static constexpr auto disconnect_pipe         = CTL_CODE(FILE_DEVICE_CONSOLE, 6,  METHOD_NEITHER,     FILE_ANY_ACCESS);
                    static constexpr auto set_server_information  = CTL_CODE(FILE_DEVICE_CONSOLE, 7,  METHOD_NEITHER,     FILE_ANY_ACCESS);
                    static constexpr auto get_server_pid          = CTL_CODE(FILE_DEVICE_CONSOLE, 8,  METHOD_NEITHER,     FILE_ANY_ACCESS);
                    static constexpr auto get_display_size        = CTL_CODE(FILE_DEVICE_CONSOLE, 9,  METHOD_NEITHER,     FILE_ANY_ACCESS);
                    static constexpr auto update_display          = CTL_CODE(FILE_DEVICE_CONSOLE, 10, METHOD_NEITHER,     FILE_ANY_ACCESS);
                    static constexpr auto set_cursor              = CTL_CODE(FILE_DEVICE_CONSOLE, 11, METHOD_NEITHER,     FILE_ANY_ACCESS);
                    static constexpr auto allow_via_uiaccess      = CTL_CODE(FILE_DEVICE_CONSOLE, 12, METHOD_NEITHER,     FILE_ANY_ACCESS);
                    static constexpr auto launch_server           = CTL_CODE(FILE_DEVICE_CONSOLE, 13, METHOD_NEITHER,     FILE_ANY_ACCESS);
                    static constexpr auto get_font_size           = CTL_CODE(FILE_DEVICE_CONSOLE, 14, METHOD_NEITHER,     FILE_ANY_ACCESS);
                };
                struct inmode
                {
                    static constexpr auto preprocess    = 0x0001; // ENABLE_PROCESSED_INPUT
                    static constexpr auto cooked        = 0x0002; // ENABLE_LINE_INPUT
                    static constexpr auto echo          = 0x0004; // ENABLE_ECHO_INPUT
                    static constexpr auto winsize       = 0x0008; // ENABLE_WINDOW_INPUT
                    static constexpr auto mouse         = 0x0010; // ENABLE_MOUSE_INPUT
                    static constexpr auto insert        = 0x0020; // ENABLE_INSERT_MODE
                    static constexpr auto quickedit     = 0x0040; // ENABLE_QUICK_EDIT_MODE
                    static constexpr auto extended      = 0x0080; // ENABLE_EXTENDED_FLAGS
                    static constexpr auto auto_position = 0x0100; // ENABLE_AUTO_POSITION
                    static constexpr auto vt            = 0x0200; // ENABLE_VIRTUAL_TERMINAL_INPUT
                };
                struct outmode
                {
                    static constexpr auto preprocess    = 0x0001; // ENABLE_PROCESSED_OUTPUT
                    static constexpr auto wrap_at_eol   = 0x0002; // ENABLE_WRAP_AT_EOL_OUTPUT
                    static constexpr auto vt            = 0x0004; // ENABLE_VIRTUAL_TERMINAL_PROCESSING
                    static constexpr auto no_auto_cr    = 0x0008; // DISABLE_NEWLINE_AUTO_RETURN
                    static constexpr auto lvb_grid      = 0x0010; // ENABLE_LVB_GRID_WORLDWIDE
                };

                static auto handle(view rootpath)
                {
                    return nt::object(rootpath,
                                      GENERIC_ALL,
                                      OBJ_CASE_INSENSITIVE | OBJ_INHERIT);
                }
                static auto handle(fd_t server, view relpath, bool inheritable = {})
                {
                    return nt::object(relpath,
                                      GENERIC_READ | GENERIC_WRITE | SYNCHRONIZE,
                                      OBJ_CASE_INSENSITIVE | (inheritable ? OBJ_INHERIT : 0),
                                      FILE_SYNCHRONOUS_IO_NONALERT,
                                      server);
                }
                static auto handle(fd_t cloned_handle)
                {
                    auto handle_clone = INVALID_FD;
                    auto ok = ::DuplicateHandle(::GetCurrentProcess(),
                                                cloned_handle,
                                                ::GetCurrentProcess(),
                                               &handle_clone,
                                                0,
                                                TRUE,
                                                DUPLICATE_SAME_ACCESS);
                    if (ok) return handle_clone;
                    else
                    {
                        log("  os: unexpected result when duplicate system object handle, errcode ", os::error());
                        return INVALID_FD;
                    }
                }
            };
        };

    #endif

    struct file
    {
        fd_t r; // file: Read descriptor.
        fd_t w; // file: Send descriptor.

        operator bool ()    { return r != INVALID_FD
                                  && w != INVALID_FD; }
        void close()
        {
            if (w == r)
            {
                os::close(r);
                w = r;
            }
            else
            {
                os::close(w);
                os::close(r);
            }
        }
        void shutdown() // Reset writing end of the pipe to interrupt reading call.
        {
            if (w == r)
            {
                os::close(w);
                r = w;
            }
            else
            {
                os::close(w);
            }
        }
        void shutsend() // Reset writing end of the pipe to interrupt reading call.
        {
            os::close(w);
        }
        friend auto& operator << (std::ostream& s, file const& handle)
        {
            return s << handle.r << "," << handle.w;
        }
        auto& operator = (file&& f)
        {
            r = f.r;
            w = f.w;
            f.r = INVALID_FD;
            f.w = INVALID_FD;
            return *this;
        }

        file(file const&) = delete;
        file(file&& f)
            : r{ f.r },
              w{ f.w }
        {
            f.r = INVALID_FD;
            f.w = INVALID_FD;
        }
        file(fd_t r = INVALID_FD, fd_t w = INVALID_FD)
            : r{ r },
              w{ w }
        { }
       ~file()
        {
            close();
        }
    };

    template<class SIZE_T>
    auto recv(fd_t fd, char* buff, SIZE_T size)
    {
        #if defined(_WIN32)

            auto count = DWORD{};
            auto fSuccess = ::ReadFile( fd,       // pipe handle
                                        buff,     // buffer to receive reply
                                (DWORD) size,     // size of buffer
                                       &count,    // number of bytes read
                                        nullptr); // not overlapped
            if (!fSuccess) count = 0;

        #else

            auto count = ::read(fd, buff, size);

        #endif

        return count > 0 ? qiew{ buff, count }
                         : qiew{}; // An empty result is always an error condition.
    }
    template<bool IS_TTY = true, class SIZE_T>
    auto send(fd_t fd, char const* buff, SIZE_T size)
    {
        while (size)
        {
            #if defined(_WIN32)

                auto count = DWORD{};
                auto fSuccess = ::WriteFile( fd,       // pipe handle
                                             buff,     // message
                                     (DWORD) size,     // message length
                                            &count,    // bytes written
                                             nullptr); // not overlapped

            #else
                // Mac OS X does not support the flag MSG_NOSIGNAL
                // See GH#182, https://lists.apple.com/archives/macnetworkprog/2002/Dec/msg00091.html
                #if defined(__APPLE__)
                    #define NO_SIGSEND SO_NOSIGPIPE
                #else
                    #define NO_SIGSEND MSG_NOSIGNAL
                #endif

                auto count = IS_TTY ? ::write(fd, buff, size)              // recursive connection causes sigpipe on destroy when using write(2) despite using ::signal(SIGPIPE, SIG_IGN)
                                    : ::send (fd, buff, size, NO_SIGSEND); // ::send() does not work with ::open_pty() and ::pipe() (Errno 88 - it is not a socket)

                #undef NO_SIGSEND
                //  send(2) does not work with file descriptors, only sockets.
                // write(2) works with fds as well as sockets.

            #endif

            if (count != size)
            {
                if (count > 0)
                {
                    //todo stackoverflow in dtvt mode
                    //log("send: partial writing: socket=", fd,
                    //    " total=", size, ", written=", count);
                    buff += count;
                    size -= count;
                }
                else
                {
                    //todo stackoverflow in dtvt mode
                    //log("send: aborted write to socket=", fd, " count=", count, " size=", size, " IS_TTY=", IS_TTY ?"true":"faux");
                    return faux;
                }
            }
            else return true;
        }
        return faux;
    }
    template<bool IS_TTY = true, class T, class SIZE_T>
    auto send(fd_t fd, T* buff, SIZE_T size)
    {
        return os::send<IS_TTY, SIZE_T>(fd, (char const*)buff, size);
    }
    template<bool IS_TTY = true>
    auto send(fd_t fd, view buff)
    {
        return os::send<IS_TTY>(fd, buff.data(), buff.size());
    }
    template<class ...Args>
    auto recv(file& handle, Args&&... args)
    {
        return recv(handle.r, std::forward<Args>(args)...);
    }
    template<class ...Args>
    auto send(file& handle, Args&&... args)
    {
        return send(handle.w, std::forward<Args>(args)...);
    }

    namespace
    {
        #if defined(_WIN32)

            template<class A, std::size_t... I>
            constexpr auto _repack(fd_t h, A const& a, std::index_sequence<I...>)
            {
                return std::array{ a[I]..., h };
            }
            template<std::size_t N, class P, class IDX = std::make_index_sequence<N>, class ...Args>
            constexpr auto _combine(std::array<fd_t, N> const& a, fd_t h, P&& proc, Args&&... args)
            {   // Clang 11.0.1 don't allow sizeof...(args) as bool
                if constexpr (!!sizeof...(args)) return _combine(_repack(h, a, IDX{}), std::forward<Args>(args)...);
                else                             return _repack(h, a, IDX{});
            }
            template<class P, class ...Args>
            constexpr auto _fd_set(fd_t handle, P&& proc, Args&&... args)
            {
                if constexpr (!!sizeof...(args)) return _combine(std::array{ handle }, std::forward<Args>(args)...);
                else                             return std::array{ handle };
            }
            template<class R, class P, class ...Args>
            constexpr auto _handle(R i, fd_t handle, P&& proc, Args&&... args)
            {
                if (i == 0)
                {
                    proc();
                    return true;
                }
                else
                {
                    if constexpr (!!sizeof...(args)) return _handle(--i, std::forward<Args>(args)...);
                    else                             return faux;
                }
            }

        #else

            template<class P, class ...Args>
            auto _fd_set(fd_set& socks, fd_t handle, P&& proc, Args&&... args)
            {
                FD_SET(handle, &socks);
                if constexpr (!!sizeof...(args))
                {
                    return std::max(handle, _fd_set(socks, std::forward<Args>(args)...));
                }
                else
                {
                    return handle;
                }
            }
            template<class P, class ...Args>
            auto _select(fd_set& socks, fd_t handle, P&& proc, Args&&... args)
            {
                if (FD_ISSET(handle, &socks))
                {
                    proc();
                }
                else
                {
                    if constexpr (!!sizeof...(args)) _select(socks, std::forward<Args>(args)...);
                }
            }

        #endif
    }

    template<bool NONBLOCKED = faux, class ...Args>
    void select(Args&&... args)
    {
        #if defined(_WIN32)

            static constexpr auto timeout = NONBLOCKED ? 0 /*milliseconds*/ : INFINITE;
            auto socks = _fd_set(std::forward<Args>(args)...);

            // Note: ::WaitForMultipleObjects() does not work with pipes (DirectVT).
            auto yield = ::WaitForMultipleObjects((DWORD)socks.size(), socks.data(), FALSE, timeout);
            yield -= WAIT_OBJECT_0;
            _handle(yield, std::forward<Args>(args)...);

        #else

            auto timeval = ::timeval{ .tv_sec = 0, .tv_usec = 0 };
            auto timeout = NONBLOCKED ? &timeval/*returns immediately*/ : nullptr;
            auto socks = fd_set{};
            FD_ZERO(&socks);

            auto nfds = 1 + _fd_set(socks, std::forward<Args>(args)...);
            if (::select(nfds, &socks, 0, 0, timeout) > 0)
            {
                _select(socks, std::forward<Args>(args)...);
            }

        #endif
    }

    class legacy
    {
    public:
        static constexpr auto clean  = 0;
        static constexpr auto mouse  = 1 << 0;
        static constexpr auto vga16  = 1 << 1;
        static constexpr auto vga256 = 1 << 2;
        static constexpr auto direct = 1 << 3;
        static auto& get_state()
        {
            static auto state = faux;
            return state;
        }
        static auto& get_start()
        {
            static auto start = text{};
            return start;
        }
        static auto& get_setup()
        {
            static auto setup = text{};
            return setup;
        }
        static auto& get_ready()
        {
            static auto ready = faux;
            return ready;
        }
        template<class T>
        static auto str(T mode)
        {
            auto result = text{};
            if (mode)
            {
                if (mode & mouse ) result += "mouse ";
                if (mode & vga16 ) result += "vga16 ";
                if (mode & vga256) result += "vga256 ";
                if (mode & direct) result += "direct ";
                if (result.size()) result.pop_back();
            }
            else result = "fresh";
            return result;
        }
        static void send_dmd(fd_t m_pipe_w, view config)
        {
            auto buffer = ansi::dtvt::binary::marker{ config.size() };
            os::send<true>(m_pipe_w, buffer.data, buffer.size);
        }
        static auto peek_dmd(fd_t stdin_fd)
        {
            auto& ready = get_ready();
            auto& state = get_state();
            auto& start = get_start();
            auto& setup = get_setup();
            auto cfsize = size_t{};
            if (ready) return state;
            ready = true;
            #if defined(_WIN32)

                // ::WaitForMultipleObjects() does not work with pipes (DirectVT).
                auto buffer = ansi::dtvt::binary::marker{};
                auto length = DWORD{ 0 };
                if (::PeekNamedPipe(stdin_fd,       // hNamedPipe
                                    &buffer,        // lpBuffer
                                    sizeof(buffer), // nBufferSize
                                    &length,        // lpBytesRead
                                    NULL,           // lpTotalBytesAvail,
                                    NULL))          // lpBytesLeftThisMessage
                {
                    if (length)
                    {
                        state = buffer.size == length && buffer.get_sz(cfsize);
                        if (state)
                        {
                            os::recv(stdin_fd, buffer.data, buffer.size);
                        }
                    }
                }

            #else

                os::select<true>(stdin_fd, [&]
                {
                    auto buffer = ansi::dtvt::binary::marker{};
                    auto header = os::recv(stdin_fd, buffer.data, buffer.size);
                    auto length = header.length();
                    if (length)
                    {
                        state = buffer.size == length && buffer.get_sz(cfsize);
                        if (!state)
                        {
                            start = header; //todo use it when the reading thread starts
                        }
                    }
                });

            #endif
            if (cfsize)
            {
                setup.resize(cfsize);
                auto buffer = setup.data();
                while (cfsize)
                {
                    if (auto crop = os::recv(stdin_fd, buffer, cfsize))
                    {
                        cfsize -= crop.size();
                        buffer += crop.size();
                    }
                    else
                    {
                        state = faux;
                        break;
                    }
                }
            }
            return state;
        }
    };

    void exit(int code)
    {
        #if defined(_WIN32)
            ::ExitProcess(code);
        #else
            if (is_daemon) ::closelog();
            ::exit(code);
        #endif
    }
    template<class ...Args>
    void exit(int code, Args&&... args)
    {
        log(args...);
        os::exit(code);
    }

    namespace env
    {
        // os::env: Get envvar value.
        auto get(view variable)
        {
            auto var = text{ variable };
            auto val = std::getenv(var.c_str());
            return val ? text{ val }
                       : text{};
        }
        // os::env: Set envvar value.
        auto set(view variable, view value)
        {
            auto var = text{ variable };
            auto val = text{ value    };
            #if defined(_WIN32)
                ok(::SetEnvironmentVariableA(var.c_str(), val.c_str()), "::SetEnvironmentVariableA unexpected result");
            #else
                ok(::setenv(var.c_str(), val.c_str(), 1), "::setenv unexpected result");
            #endif
        }
        // os::env: Get list of envvars using wildcard.
        auto list(text&& var)
        {
            auto crop = std::vector<text>{};
            auto list = environ;
            while (*list)
            {
                auto v = view{ *list++ };
                if (v.starts_with(var))
                {
                    crop.emplace_back(v);
                }
            }
            std::sort(crop.begin(), crop.end());
            return crop;
        }
    }

    text get_shell()
    {
        #if defined(_WIN32)

            return "cmd";

        #else

            auto shell = os::env::get("SHELL");
            if (shell.empty()
             || shell.ends_with("vtm"))
            {
                shell = "bash"; //todo request it from user if empty; or make it configurable
                log("  os: using '", shell, "' as a fallback login shell");
            }
            return shell;

        #endif
    }
    auto homepath()
    {
        #if defined(_WIN32)
            return fs::path{ os::env::get("HOMEDRIVE") } / fs::path{ os::env::get("HOMEPATH") };
        #else
            return fs::path{ os::env::get("HOME") };
        #endif
    }
    auto local_mode()
    {
        auto conmode = -1;
        #if defined(__linux__)

            if (-1 != ::ioctl(STDOUT_FD, KDGETMODE, &conmode))
            {
                switch (conmode)
                {
                    case KD_TEXT:     break;
                    case KD_GRAPHICS: break;
                    default:          break;
                }
            }

        #endif
        return conmode != -1;
    }
    auto vt_mode()
    {
        auto mode = si32{ legacy::clean };
        if (os::legacy::peek_dmd(STDIN_FD))
        {
            log("  os: DirectVT detected");
            mode |= legacy::direct;
        }
        else
        {
            #if defined(_WIN32) // Set vt-mode and UTF-8 codepage unconditionally.

                auto outmode = DWORD{};
                if(::GetConsoleMode(STDOUT_FD, &outmode))
                {
                    outmode |= nt::console::outmode::vt;
                    ::SetConsoleMode(STDOUT_FD, outmode);
                    ::SetConsoleOutputCP(65001);
                    ::SetConsoleCP(65001);
                }

            #endif
            if (auto term = os::env::get("TERM"); term.size())
            {
                log("  os: terminal type \"", term, "\"");

                auto vga16colors = { // https://github.com//termstandard/colors
                    "ansi",
                    "linux",
                    "xterm-color",
                    "dvtm", //todo track: https://github.com/martanne/dvtm/issues/10
                    "fbcon",
                };
                auto vga256colors = {
                    "rxvt-unicode-256color",
                };

                if (term.ends_with("16color") || term.ends_with("16colour"))
                {
                    mode |= legacy::vga16;
                }
                else
                {
                    for (auto& type : vga16colors)
                    {
                        if (term == type)
                        {
                            mode |= legacy::vga16;
                            break;
                        }
                    }
                    if (!mode)
                    {
                        for (auto& type : vga256colors)
                        {
                            if (term == type)
                            {
                                mode |= legacy::vga256;
                                break;
                            }
                        }
                    }
                }

                if (os::env::get("TERM_PROGRAM") == "Apple_Terminal")
                {
                    log("  os: macOS Apple_Terminal detected");
                    if (!(mode & legacy::vga16)) mode |= legacy::vga256;
                }

                if (os::local_mode()) mode |= legacy::mouse;

                log("  os: color mode: ", mode & legacy::vga16  ? "16-color"
                                        : mode & legacy::vga256 ? "256-color"
                                                                : "true-color");
                log("  os: mouse mode: ", mode & legacy::mouse ? "console" : "VT-style");
            }
        }
        return mode;
    }
    auto vgafont_update(si32 mode)
    {
        #if defined(__linux__)

            if (mode & legacy::mouse)
            {
                auto chars = std::vector<unsigned char>(512 * 32 * 4);
                auto fdata = console_font_op{ .op        = KD_FONT_OP_GET,
                                              .flags     = 0,
                                              .width     = 32,
                                              .height    = 32,
                                              .charcount = 512,
                                              .data      = chars.data() };
                if (!ok(::ioctl(STDOUT_FD, KDFONTOP, &fdata), "first KDFONTOP + KD_FONT_OP_GET failed")) return;

                auto slice_bytes = (fdata.width + 7) / 8;
                auto block_bytes = (slice_bytes * fdata.height + 31) / 32 * 32;
                auto tophalf_idx = 10;
                auto lowhalf_idx = 254;
                auto tophalf_ptr = fdata.data + block_bytes * tophalf_idx;
                auto lowhalf_ptr = fdata.data + block_bytes * lowhalf_idx;
                for (auto row = 0; row < fdata.height; row++)
                {
                    auto is_top = row < fdata.height / 2;
                   *tophalf_ptr = is_top ? 0xFF : 0x00;
                   *lowhalf_ptr = is_top ? 0x00 : 0xFF;
                    tophalf_ptr+= slice_bytes;
                    lowhalf_ptr+= slice_bytes;
                }
                fdata.op = KD_FONT_OP_SET;
                if (!ok(::ioctl(STDOUT_FD, KDFONTOP, &fdata), "second KDFONTOP + KD_FONT_OP_SET failed")) return;

                auto max_sz = std::numeric_limits<unsigned short>::max();
                auto spairs = std::vector<unipair>(max_sz);
                auto dpairs = std::vector<unipair>(max_sz);
                auto srcmap = unimapdesc{ max_sz, spairs.data() };
                auto dstmap = unimapdesc{ max_sz, dpairs.data() };
                auto dstptr = dstmap.entries;
                auto srcptr = srcmap.entries;
                if (!ok(::ioctl(STDOUT_FD, GIO_UNIMAP, &srcmap), "ioctl(STDOUT_FD, GIO_UNIMAP) failed")) return;
                auto srcend = srcmap.entries + srcmap.entry_ct;
                while (srcptr != srcend) // Drop 10, 211, 254 and 0x2580▀ + 0x2584▄.
                {
                    auto& smap = *srcptr++;
                    if (smap.fontpos != 10
                     && smap.fontpos != 211
                     && smap.fontpos != 254
                     && smap.unicode != 0x2580
                     && smap.unicode != 0x2584) *dstptr++ = smap;
                }
                dstmap.entry_ct = dstptr - dstmap.entries;
                unipair new_recs[] = { { 0x2580,  10 },
                                       { 0x2219, 211 },
                                       { 0x2022, 211 },
                                       { 0x25CF, 211 },
                                       { 0x25A0, 254 },
                                       { 0x25AE, 254 },
                                       { 0x2584, 254 } };
                if (dstmap.entry_ct < max_sz - std::size(new_recs)) // Add new records.
                {
                    for (auto& p : new_recs) *dstptr++ = p;
                    dstmap.entry_ct += std::size(new_recs);
                    if (!ok(::ioctl(STDOUT_FD, PIO_UNIMAP, &dstmap), "ioctl(STDOUT_FD, PIO_UNIMAP) failed")) return;
                }
                else log("  os: vgafont_update failed - UNIMAP is full");
            }

        #endif
    }
    auto vtgafont_revert()
    {
        //todo implement
    }
    template<bool NameOnly = faux>
    auto current_module_file()
    {
        auto result = text{};
        #if defined(_WIN32)

            auto handle = ::GetCurrentProcess();
            auto buffer = std::vector<char>(MAX_PATH);

            while (buffer.size() <= 32768)
            {
                auto length = ::GetModuleFileNameEx(handle,         // hProcess
                                                    NULL,           // hModule
                                                    buffer.data(),  // lpFilename
                                 static_cast<DWORD>(buffer.size()));// nSize
                if (length == 0) break;

                if (buffer.size() > length + 1)
                {
                    result = text(buffer.data(), length);
                    break;
                }

                buffer.resize(buffer.size() << 1);
            }

        #elif defined(__linux__) || defined(__NetBSD__)

            auto path = ::realpath("/proc/self/exe", nullptr);
            if (path)
            {
                result = text(path);
                ::free(path);
            }

        #elif defined(__APPLE__)

            auto size = uint32_t{};
            if (-1 == ::_NSGetExecutablePath(nullptr, &size))
            {
                auto buff = std::vector<char>(size);
                if (0 == ::_NSGetExecutablePath(buff.data(), &size))
                {
                    auto path = ::realpath(buff.data(), nullptr);
                    if (path)
                    {
                        result = text(path);
                        ::free(path);
                    }
                }
            }

        #elif defined(__FreeBSD__)

            auto size = 0_sz;
            int  name[] = { CTL_KERN, KERN_PROC, KERN_PROC_PATHNAME, -1 };
            if (::sysctl(name, std::size(name), nullptr, &size, nullptr, 0) == 0)
            {
                auto buff = std::vector<char>(size);
                if (::sysctl(name, std::size(name), buff.data(), &size, nullptr, 0) == 0)
                {
                    result = text(buff.data(), size);
                }
            }

        #endif
        #if not defined(_WIN32)

            if (result.empty())
            {
                      auto path = ::realpath("/proc/self/exe",        nullptr);
                if (!path) path = ::realpath("/proc/curproc/file",    nullptr);
                if (!path) path = ::realpath("/proc/self/path/a.out", nullptr);
                if (path)
                {
                    result = text(path);
                    ::free(path);
                }
            }

        #endif
        if (result.empty())
        {
            os::fail("can't get current module file path, fallback to '", DESKTOPIO_MYPATH, "`");
            result = DESKTOPIO_MYPATH;
        }
        if constexpr (NameOnly)
        {
            auto code = std::error_code{};
            auto file = fs::directory_entry(result, code);
            if (!code)
            {
                result = file.path().filename().string();
            }
        }
        auto c = result.front();
        if (c != '\"' && c != '\'' && result.find(' ') != text::npos)
        {
            result = '\"' + result + '\"';
        }
        return result;
    }
    auto execvp(text cmdline)
    {
        #if defined(_WIN32)
        #else

            if (auto args = os::args::split(cmdline); args.size())
            {
                auto& binary = args.front();
                if (binary.size() > 2) // Remove quotes,
                {
                    auto c = binary.front();
                    if (binary.back() == c && (c == '\"' || c == '\''))
                    {
                        binary = binary.substr(1, binary.size() - 2);
                    }
                }
                auto argv = std::vector<char*>{};
                for (auto& c : args)
                {
                    argv.push_back(c.data());
                }
                argv.push_back(nullptr);
                ::execvp(argv.front(), argv.data());
            }

        #endif
    }
    template<bool Logs = true>
    auto exec(text cmdline)
    {
        if constexpr (Logs) log("exec: '" + cmdline + "'");
        #if defined(_WIN32)
            
            auto shadow = view{ cmdline };
            auto binary = utf::to_utf(utf::get_token(shadow));
            auto params = utf::to_utf(shadow);
            auto ShExecInfo = ::SHELLEXECUTEINFOW{};
            ShExecInfo.cbSize = sizeof(::SHELLEXECUTEINFOW);
            ShExecInfo.fMask = SEE_MASK_NOCLOSEPROCESS;
            ShExecInfo.hwnd = NULL;
            ShExecInfo.lpVerb = NULL;
            ShExecInfo.lpFile = binary.c_str();
            ShExecInfo.lpParameters = params.c_str();
            ShExecInfo.lpDirectory = NULL;
            ShExecInfo.nShow = 0;
            ShExecInfo.hInstApp = NULL;
            if (::ShellExecuteExW(&ShExecInfo)) return true;

        #else

            auto p_id = ::fork();
            if (p_id == 0) // Child branch.
            {
                os::execvp(cmdline);
                auto errcode = errno;
                if constexpr (Logs) os::fail("exec: failed to spawn '", cmdline, "'");
                os::exit(errcode);
            }
            else if (p_id > 0) // Parent branch.
            {
                auto stat = int{};
                ::waitpid(p_id, &stat, 0); // Wait for the child to avoid zombies.
                if (WIFEXITED(stat) && (WEXITSTATUS(stat) == 0))
                {
                    return true; // Child forked and exited successfully.
                }
            }

        #endif
        if constexpr (Logs) os::fail("exec: failed to spawn '", cmdline, "'");
        return faux;
    }
    void start_log(text srv_name)
    {
        is_daemon = true;
        #if defined(_WIN32)
            //todo implement
        #else
            ::openlog(srv_name.c_str(), LOG_NOWAIT | LOG_PID, LOG_USER);
        #endif
    }
    void stdlog(view data)
    {
        static auto logs = ansi::dtvt::binary::debuglogs_t{};
        //todo view -> text
        logs.set(text{ data });
        logs.send([&](auto& block){ os::send(STDOUT_FD, block.data(), block.size()); });
        //std::cerr << data;
        //os::send<true>(STDERR_FD, data.data(), data.size());
    }
    void syslog(view data)
    {
        if (os::is_daemon)
        {
            #if defined(_WIN32)
                //todo implement
            #else
                auto copy = text{ data };
                ::syslog(LOG_NOTICE, "%s", copy.c_str());
            #endif
        }
        else std::cout << data << std::flush;
    }
    auto daemonize(text params)
    {
        #if defined(_WIN32)

            auto srv_name = os::current_module_file();
            if (os::exec<true>(srv_name + " " + params))
            {
                os::exit(0); // Child forked successfully.
            }

        #else

            auto pid = ::fork();
            if (pid < 0)
            {
                os::exit(1, "daemon: fork error");
            }
            else if (pid == 0) // Child process.
            {
                ::setsid(); // Make this process the session leader of a new session.
                pid = ::fork();
                if (pid < 0)
                {
                    os::exit(1, "daemon: fork error");
                }
                else if (pid == 0) // GrandChild process.
                {
                    ::umask(0);
                    auto srv_name = os::current_module_file();
                    os::start_log(srv_name);
                    ::close(STDIN_FD);  // A daemon cannot use terminal,
                    ::close(STDOUT_FD); // so close standard file descriptors
                    ::close(STDERR_FD); // for security reasons.
                    return true;
                }
                os::exit(0); // SUCCESS (This child is reaped below with waitpid()).
            }

            // Reap the child, leaving the grandchild to be inherited by init.
            auto stat = int{};
            ::waitpid(pid, &stat, 0);
            if (WIFEXITED(stat) && (WEXITSTATUS(stat) == 0))
            {
                os::exit(0); // Child forked and exited successfully.
            }

        #endif
        return faux;
    }
    auto host_name()
    {
        auto hostname = text{};
        #if defined(_WIN32)

            auto dwSize = DWORD{ 0 };
            ::GetComputerNameEx(::ComputerNamePhysicalDnsFullyQualified, NULL, &dwSize);

            if (dwSize)
            {
                auto buffer = std::vector<char>(dwSize);
                if (::GetComputerNameEx(::ComputerNamePhysicalDnsFullyQualified, buffer.data(), &dwSize))
                {
                    if (dwSize && buffer[dwSize - 1] == 0) dwSize--;
                    hostname = text(buffer.data(), dwSize);
                }
            }

        #else

            // APPLE: AI_CANONNAME undeclared
            //std::vector<char> buffer(MAXHOSTNAMELEN);
            //if (0 == gethostname(buffer.data(), buffer.size()))
            //{
            //	struct addrinfo hints, * info;
            //
            //	::memset(&hints, 0, sizeof hints);
            //	hints.ai_family = AF_UNSPEC;
            //	hints.ai_socktype = SOCK_STREAM;
            //	hints.ai_flags = AI_CANONNAME | AI_CANONIDN;
            //
            //	if (0 == getaddrinfo(buffer.data(), "http", &hints, &info))
            //	{
            //		hostname = std::string(info->ai_canonname);
            //		//for (auto p = info; p != NULL; p = p->ai_next)
            //		//{
            //		//	hostname = std::string(p->ai_canonname);
            //		//}
            //		freeaddrinfo(info);
            //	}
            //}

        #endif
        return hostname;
    }
    auto is_mutex_exists(text&& mutex_name)
    {
        auto result = faux;
        #if defined(_WIN32)

            auto mutex = ::CreateMutexA(0, 0, mutex_name.c_str());
            result = ::GetLastError() == ERROR_ALREADY_EXISTS;
            ::CloseHandle(mutex);

        #else

            //todo linux implementation
            result = true;

        #endif
        return result;
    }
    auto process_id()
    {
        auto result = ui32{};
        #if defined(_WIN32)
            result = ::GetCurrentProcessId();
        #else
            result = ::getpid();
        #endif
        return result;
    }
    static auto logged_in_users(view domain_delimiter = "\\", view record_delimiter = "\0") //  static required by ::WTSQuerySessionInformation
    {
        auto active_users_array = text{};
        #if defined(_WIN32)

            auto SessionInfo_pointer = PWTS_SESSION_INFO{};
            auto count = DWORD{};
            if (::WTSEnumerateSessions(WTS_CURRENT_SERVER_HANDLE, 0, 1, &SessionInfo_pointer, &count))
            {
                for (DWORD i = 0; i < count; i++)
                {
                    auto si = SessionInfo_pointer[i];
                    auto buffer_pointer = LPTSTR{};
                    auto buffer_length = DWORD{};

                    ::WTSQuerySessionInformation(WTS_CURRENT_SERVER_HANDLE, si.SessionId, WTSUserName, &buffer_pointer, &buffer_length);
                    auto user = text(utf::to_view(buffer_pointer, buffer_length));
                    ::WTSFreeMemory(buffer_pointer);

                    if (user.length())
                    {
                        ::WTSQuerySessionInformation(WTS_CURRENT_SERVER_HANDLE, si.SessionId, WTSDomainName, &buffer_pointer, &buffer_length);
                        auto domain = text(utf::to_view(buffer_pointer, buffer_length / sizeof(wchr)));
                        ::WTSFreeMemory(buffer_pointer);

                        active_users_array += domain;
                        active_users_array += domain_delimiter;
                        active_users_array += user;
                        active_users_array += domain_delimiter;
                        active_users_array += "local";
                        active_users_array += record_delimiter;
                    }
                }
                ::WTSFreeMemory(SessionInfo_pointer);
                if (active_users_array.size())
                {
                    active_users_array = utf::remove(active_users_array, record_delimiter);
                }
            }

        #else

            static constexpr auto NAME_WIDTH = 8;

            // APPLE: utmp is deprecated

            // if (FILE* ufp = ::fopen(_PATH_UTMP, "r"))
            // {
            // 	struct utmp usr;
            //
            // 	while (::fread((char*)&usr, sizeof(usr), 1, ufp) == 1)
            // 	{
            // 		if (*usr.ut_user && *usr.ut_host && *usr.ut_line && *usr.ut_line != '~')
            // 		{
            // 			active_users_array += usr.ut_host;
            // 			active_users_array += domain_delimiter;
            // 			active_users_array += usr.ut_user;
            // 			active_users_array += domain_delimiter;
            // 			active_users_array += usr.ut_line;
            // 			active_users_array += record_delimiter;
            // 		}
            // 	}
            // 	::fclose(ufp);
            // }

        #endif
        return active_users_array;
    }
    auto user()
    {
        #if defined(_WIN32)

            static constexpr auto INFO_BUFFER_SIZE = 32767UL;
            char infoBuf[INFO_BUFFER_SIZE];
            auto bufCharCount = DWORD{ INFO_BUFFER_SIZE };

            if (::GetUserNameA(infoBuf, &bufCharCount))
            {
                if (bufCharCount && infoBuf[bufCharCount - 1] == 0)
                {
                    bufCharCount--;
                }
                return text(infoBuf, bufCharCount);
            }
            else
            {
                os::fail("GetUserName error");
                return text{};
            }

        #else

            uid_t id;
            id = ::geteuid();
            return id;

        #endif
    }
    auto set_clipboard(view mime, view utf8)
    {
        // Generate the following formats:
        //   clip::textonly | clip::disabled
        //        CF_UNICODETEXT: Raw UTF-16
        //               cf_ansi: ANSI-text UTF-8 with mime mark
        //   clip::ansitext
        //               cf_rich: RTF-group UTF-8
        //        CF_UNICODETEXT: ANSI-text UTF-16
        //               cf_ansi: ANSI-text UTF-8 with mime mark
        //   clip::richtext
        //               cf_rich: RTF-group UTF-8
        //        CF_UNICODETEXT: Plaintext UTF-16
        //               cf_ansi: ANSI-text UTF-8 with mime mark
        //   clip::htmltext
        //               cf_ansi: ANSI-text UTF-8 with mime mark
        //               cf_html: HTML-code UTF-8
        //        CF_UNICODETEXT: HTML-code UTF-16
        //   clip::safetext (https://learn.microsoft.com/en-us/windows/win32/dataxchg/clipboard-formats#cloud-clipboard-and-clipboard-history-formats)
        //    ExcludeClipboardContentFromMonitorProcessing: 1
        //                    CanIncludeInClipboardHistory: 0
        //                       CanUploadToCloudClipboard: 0
        //                                  CF_UNICODETEXT: Raw UTF-16
        //                                         cf_ansi: ANSI-text UTF-8 with mime mark
        //
        //  cf_ansi format: payload=mime_type/size_x/size_y;utf8_data

        using ansi::clip;

        auto success = faux;
        auto size = twod{ 80,25 };
        {
            auto i = 1;
            utf::divide<feed::rev>(mime, '/', [&](auto frag)
            {
                if (auto v = utf::to_int(frag)) size[i] = v.value();
                return i--;
            });
        }

        #if defined(_WIN32)

            auto send = [&](auto cf_format, view data)
            {
                auto _send = [&](auto const& data)
                {
                    auto size = (data.size() + 1/*null terminator*/) * sizeof(*(data.data()));
                    if (auto gmem = ::GlobalAlloc(GMEM_MOVEABLE, size))
                    {
                        if (auto dest = ::GlobalLock(gmem))
                        {
                            std::memcpy(dest, data.data(), size);
                            ::GlobalUnlock(gmem);
                            ok(::SetClipboardData(cf_format, gmem) && (success = true), "unexpected result from ::SetClipboardData cf_format=" + std::to_string(cf_format));
                        }
                        else log("  os: ::GlobalLock returns unexpected result");
                        ::GlobalFree(gmem);
                    }
                    else log("  os: ::GlobalAlloc returns unexpected result");
                };
                cf_format == os::cf_text ? _send(utf::to_utf(data))
                                         : _send(data);
            };

            auto lock = std::lock_guard{ os::clipboard_mutex };
            ok(::OpenClipboard(nullptr), "::OpenClipboard");
            ok(::EmptyClipboard(), "::EmptyClipboard");
            if (utf8.size())
            {
                if (mime.size() < 5 || mime.starts_with(ansi::mimetext))
                {
                    send(os::cf_text, utf8);
                }
                else
                {
                    auto post = page{ utf8 };
                    auto info = CONSOLE_FONT_INFOEX{ sizeof(CONSOLE_FONT_INFOEX) };
                    ::GetCurrentConsoleFontEx(STDOUT_FD, faux, &info);
                    auto font = utf::to_utf(info.FaceName);
                    if (mime.starts_with(ansi::mimerich))
                    {
                        auto rich = post.to_rich(font);
                        auto utf8 = post.to_utf8();
                        send(os::cf_rich, rich);
                        send(os::cf_text, utf8);
                    }
                    else if (mime.starts_with(ansi::mimehtml))
                    {
                        auto [html, code] = post.to_html(font);
                        send(os::cf_html, html);
                        send(os::cf_text, code);
                    }
                    else if (mime.starts_with(ansi::mimeansi))
                    {
                        auto rich = post.to_rich(font);
                        send(os::cf_rich, rich);
                        send(os::cf_text, utf8);
                    }
                    else if (mime.starts_with(ansi::mimesafe))
                    {
                        send(os::cf_sec1, "1");
                        send(os::cf_sec2, "0");
                        send(os::cf_sec3, "0");
                        send(os::cf_text, utf8);
                    }
                    else
                    {
                        send(os::cf_utf8, utf8);
                    }
                }
                auto crop = ansi::add(mime, ";", utf8);
                send(os::cf_ansi, crop);
            }
            else
            {
                success = true;
            }
            ok(::CloseClipboard(), "::CloseClipboard");
            os::clipboard_sequence = ::GetClipboardSequenceNumber(); // The sequence number is incremented while closing the clipboard.

        #elif defined(__APPLE__)

            auto send = [&](auto& data)
            {
                if (auto fd = ::popen("/usr/bin/pbcopy", "w"))
                {
                    ::fwrite(data.data(), data.size(), 1, fd);
                    ::pclose(fd);
                    success = true;
                }
            };
            if (mime.starts_with(ansi::mimerich))
            {
                auto post = page{ utf8 };
                auto rich = post.to_rich();
                send(rich);
            }
            else if (mime.starts_with(ansi::mimehtml))
            {
                auto post = page{ utf8 };
                auto [html, code] = post.to_html();
                send(code);
            }
            else
            {
                send(utf8);
            }

        #else

            auto yield = ansi::esc{};
            if (mime.starts_with(ansi::mimerich))
            {
                auto post = page{ utf8 };
                auto rich = post.to_rich();
                yield.clipbuf(size, rich, clip::richtext);
            }
            else if (mime.starts_with(ansi::mimehtml))
            {
                auto post = page{ utf8 };
                auto [html, code] = post.to_html();
                yield.clipbuf(size, code, clip::htmltext);
            }
            else if (mime.starts_with(ansi::mimeansi)) //todo GH#216
            {
                yield.clipbuf(size, utf8, clip::ansitext);
            }
            else
            {
                yield.clipbuf(size, utf8, clip::textonly);
            }
            os::send<true>(STDOUT_FD, yield.data(), yield.size());
            success = true;

            #if defined(__ANDROID__)

                //todo implement

            #else

                //todo implement X11 clipboard server

            #endif

        #endif
        return success;
    }

    #if defined(_WIN32)

    /* cl.exe issue
    class security_descriptor
    {
        SECURITY_ATTRIBUTES descriptor;

    public:
        text security_string;

        operator SECURITY_ATTRIBUTES* () throw()
        {
            return &descriptor;
        }

        security_descriptor(text SSD)
            : security_string{ SSD }
        {
            ZeroMemory(&descriptor, sizeof(descriptor));
            descriptor.nLength = sizeof(descriptor);

            // four main components of a security descriptor https://docs.microsoft.com/en-us/windows/win32/secauthz/security-descriptor-string-format
            //       "O:" - owner
            //       "G:" - primary group
            //       "D:" - DACL discretionary access control list https://docs.microsoft.com/en-us/windows/desktop/SecGloss/d-gly
            //       "S:" - SACL system access control list https://docs.microsoft.com/en-us/windows/desktop/SecGloss/s-gly
            //
            // the object's owner:
            //   O:owner_sid
            //
            // the object's primary group:
            //   G:group_sid
            //
            // Security descriptor control flags that apply to the DACL:
            //   D:dacl_flags(string_ace1)(string_ace2)... (string_acen)
            //
            // Security descriptor control flags that apply to the SACL
            //   S:sacl_flags(string_ace1)(string_ace2)... (string_acen)
            //
            //   dacl_flags/sacl_flags:
            //   "P"                 SDDL_PROTECTED        The SE_DACL_PROTECTED flag is set.
            //   "AR"                SDDL_AUTO_INHERIT_REQ The SE_DACL_AUTO_INHERIT_REQ flag is set.
            //   "AI"                SDDL_AUTO_INHERITED   The SE_DACL_AUTO_INHERITED flag is set.
            //   "NO_ACCESS_CONTROL" SSDL_NULL_ACL         The ACL is null.
            //
            //   string_ace - The fields of the ACE are in the following
            //                order and are separated by semicolons (;)
            //   for syntax see https://docs.microsoft.com/en-us/windows/win32/secauthz/ace-strings
            //   ace_type;ace_flags;rights;object_guid;inherit_object_guid;account_sid;(resource_attribute)
            // ace_type:
            //  "A" SDDL_ACCESS_ALLOWED
            // ace_flags:
            //  "OI"	SDDL_OBJECT_INHERIT
            //  "CI"	SDDL_CONTAINER_INHERIT
            //  "NP"	SDDL_NO_PROPAGATE
            // rights:
            //  "GA"	SDDL_GENERIC_ALL
            // account_sid: see https://docs.microsoft.com/en-us/windows/win32/secauthz/sid-strings
            //  "SY"	SDDL_LOCAL_SYSTEM
            //  "BA"	SDDL_BUILTIN_ADMINISTRATORS
            //  "CO"	SDDL_CREATOR_OWNER
            if (!ConvertStringSecurityDescriptorToSecurityDescriptorA(
                //"D:P(A;OICI;GA;;;SY)(A;OICI;GA;;;BA)(A;OICI;GA;;;CO)",
                SSD.c_str(),
                SDDL_REVISION_1,
                &descriptor.lpSecurityDescriptor,
                NULL))
            {
                log("ConvertStringSecurityDescriptorToSecurityDescriptor error:",
                    GetLastError());
            }
        }

       ~security_descriptor()
        {
            LocalFree(descriptor.lpSecurityDescriptor);
        }
    };

    static security_descriptor global_access{ "D:P(A;OICI;GA;;;SY)(A;OICI;GA;;;BA)(A;OICI;GA;;;CO)" };
    */

    auto take_partition(text&& utf8_file_name)
    {
        auto result = text{};
        auto volume = std::vector<char>(std::max<size_t>(MAX_PATH, utf8_file_name.size() + 1));
        if (::GetVolumePathName(utf8_file_name.c_str(), volume.data(), (DWORD)volume.size()) != 0)
        {
            auto partition = std::vector<char>(50 + 1);
            if (0 != ::GetVolumeNameForVolumeMountPoint( volume.data(),
                                                      partition.data(),
                                               (DWORD)partition.size()))
            {
                result = text(partition.data());
            }
            else
            {
                //error_handler();
            }
        }
        else
        {
            //error_handler();
        }
        return result;
    }
    auto take_temp(text&& utf8_file_name)
    {
        auto tmp_dir = text{};
        auto file_guid = take_partition(std::move(utf8_file_name));
        auto i = uint8_t{ 0 };

        while (i < 100 && netxs::os::test_path(tmp_dir = file_guid + "\\$temp_" + utf::adjust(std::to_string(i++), 3, "0", true)))
        { }

        if (i == 100) tmp_dir.clear();

        return tmp_dir;
    }
    static auto trusted_domain_name() // static required by ::DsEnumerateDomainTrusts
    {
        auto info = PDS_DOMAIN_TRUSTS{};
        auto domain_name = text{};
        auto DomainCount = ULONG{};

        bool result = ::DsEnumerateDomainTrusts(nullptr, DS_DOMAIN_PRIMARY, &info, &DomainCount);
        if (result == ERROR_SUCCESS)
        {
            domain_name = text(info->DnsDomainName);
            ::NetApiBufferFree(info);
        }
        return domain_name;
    }
    static auto trusted_domain_guid() // static required by ::DsEnumerateDomainTrusts
    {
        auto info = PDS_DOMAIN_TRUSTS{};
        auto domain_guid = text{};
        auto domain_count = ULONG{};

        bool result = ::DsEnumerateDomainTrusts(nullptr, DS_DOMAIN_PRIMARY, &info, &domain_count);
        if (result == ERROR_SUCCESS && domain_count > 0)
        {
            auto& guid = info->DomainGuid;
            domain_guid = utf::to_hex(guid.Data1)
                  + '-' + utf::to_hex(guid.Data2)
                  + '-' + utf::to_hex(guid.Data3)
                  + '-' + utf::to_hex(std::string(guid.Data4, guid.Data4 + 2))
                  + '-' + utf::to_hex(std::string(guid.Data4 + 2, guid.Data4 + sizeof(guid.Data4)));

            ::NetApiBufferFree(info);
        }
        return domain_guid;
    }
    auto create_shortcut(text&& path_to_object, text&& path_to_link)
    {
        auto result = HRESULT{};
        IShellLink* psl;
        ::CoInitialize(0);
        result = ::CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, IID_IShellLink, (LPVOID*)&psl);

        if (result == S_OK)
        {
            IPersistFile* ppf;
            auto path_to_link_w = utf::to_utf(path_to_link);

            psl->SetPath(path_to_object.c_str());
            result = psl->QueryInterface(IID_IPersistFile, (LPVOID*)&ppf);
            if (SUCCEEDED(result))
            {
                result = ppf->Save(path_to_link_w.c_str(), TRUE);
                ppf->Release();
                ::CoUninitialize();
                return true;
            }
            else
            {
                //todo
                //shell_error_handler(result);
            }
            psl->Release();
        }
        else
        {
            //todo
            //shell_error_handler(result);
        }
        ::CoUninitialize();

        return faux;
    }
    auto expand(text&& directory)
    {
        auto result = text{};
        if (auto len = ::ExpandEnvironmentStrings(directory.c_str(), NULL, 0))
        {
            auto buffer = std::vector<char>(len);
            if (::ExpandEnvironmentStrings(directory.c_str(), buffer.data(), (DWORD)buffer.size()))
            {
                result = text(buffer.data());
            }
        }
        return result;
    }
    auto set_registry_key(text&& key_path, text&& parameter_name, text&& value)
    {
        auto hKey = HKEY{};
        auto status = ::RegCreateKeyEx( HKEY_LOCAL_MACHINE,
                                        key_path.c_str(),
                                        0,
                                        0,
                                        REG_OPTION_NON_VOLATILE,
                                        KEY_ALL_ACCESS,
                                        NULL,
                                        &hKey,
                                        0);

        if (status == ERROR_SUCCESS && hKey != NULL)
        {
            status = ::RegSetValueEx( hKey,
                                      parameter_name.empty() ? nullptr : parameter_name.c_str(),
                                      0,
                                      REG_SZ,
                                      (BYTE*)value.c_str(),
                                      ((DWORD)value.size() + 1) * sizeof(wchr)
            );

            ::RegCloseKey(hKey);
        }
        else
        {
            //todo
            //error_handler();
        }

        return (status == ERROR_SUCCESS);
    }
    auto get_registry_string_value(text&& key_path, text&& parameter_name)
    {
        auto result = text{};
        auto hKey = HKEY{};
        auto value_type = DWORD{};
        auto data_length = DWORD{};
        auto status = ::RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                                      key_path.c_str(),
                                      0,
                                      KEY_ALL_ACCESS,
                                      &hKey);

        if (status == ERROR_SUCCESS && hKey != NULL)
        {
            auto param = parameter_name.empty() ? nullptr : parameter_name.c_str();

            status = ::RegQueryValueEx( hKey,
                                        param,
                                        NULL,
                                        &value_type,
                                        NULL,
                                        &data_length);

            if (status == ERROR_SUCCESS && value_type == REG_SZ)
            {
                auto data = std::vector<BYTE>(data_length);
                status = ::RegQueryValueEx( hKey,
                                            param,
                                            NULL,
                                            &value_type,
                                            data.data(),
                                            &data_length);

                if (status == ERROR_SUCCESS)
                {
                    result = text(utf::to_view(reinterpret_cast<char*>(data.data()), data.size()));
                }
            }
        }
        if (status != ERROR_SUCCESS)
        {
            //todo
            //error_handler();
        }

        return result;
    }
    auto get_registry_subkeys(text&& key_path)
    {
        auto result = list{};
        auto hKey = HKEY{};
        auto status = ::RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                                      key_path.c_str(),
                                      0,
                                      KEY_ALL_ACCESS,
                                      &hKey);

        if (status == ERROR_SUCCESS && hKey != NULL)
        {
            auto lpcbMaxSubKeyLen = DWORD{};
            if (ERROR_SUCCESS == ::RegQueryInfoKey(hKey, NULL, NULL, NULL, NULL, &lpcbMaxSubKeyLen, NULL, NULL, NULL, NULL, NULL, NULL))
            {
                auto size = lpcbMaxSubKeyLen;
                auto index = DWORD{ 0 };
                auto szName = std::vector<char>(size);

                while (ERROR_SUCCESS == ::RegEnumKeyEx( hKey,
                                                        index++,
                                                        szName.data(),
                                                        &lpcbMaxSubKeyLen,
                                                        NULL,
                                                        NULL,
                                                        NULL,
                                                        NULL))
                {
                    result.push_back(text(utf::to_view(szName.data(), std::min<size_t>(lpcbMaxSubKeyLen, size))));
                    lpcbMaxSubKeyLen = size;
                }
            }

            ::RegCloseKey(hKey);
        }

        return result;
    }
    auto cmdline()
    {
        auto result = list{};
        auto argc = int{ 0 };
        auto params = std::shared_ptr<void>(::CommandLineToArgvW(GetCommandLineW(), &argc), ::LocalFree);
        auto argv = (LPWSTR*)params.get();
        for (auto i = 0; i < argc; i++)
        {
            result.push_back(utf::to_utf(argv[i]));
        }

        return result;
    }
    static auto delete_registry_tree(text&& path) // static required by ::SHDeleteKey
    {
        auto hKey = HKEY{};
        auto status = ::RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                                      path.c_str(),
                                      0,
                                      KEY_ALL_ACCESS,
                                      &hKey);

        if (status == ERROR_SUCCESS && hKey != NULL)
        {
            status = ::SHDeleteKey(hKey, path.c_str());
            ::RegCloseKey(hKey);
        }

        auto result = status == ERROR_SUCCESS;

        if (!result)
        {
            //todo
            //error_handler();
        }

        return result;
    }
    void update_process_privileges(void)
    {
        auto hToken = INVALID_FD;
        auto tp = TOKEN_PRIVILEGES{};
        auto luid = LUID{};

        if (::OpenProcessToken(::GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken))
        {
            ::LookupPrivilegeValue(NULL, SE_DEBUG_NAME, &luid);

            tp.PrivilegeCount = 1;
            tp.Privileges[0].Luid = luid;
            tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

            ::AdjustTokenPrivileges(hToken, FALSE, &tp, sizeof(TOKEN_PRIVILEGES), NULL, NULL);
        }
    }
    auto kill_process(unsigned long proc_id)
    {
        auto result = faux;

        update_process_privileges();
        auto process_handle = ::OpenProcess(SYNCHRONIZE | PROCESS_TERMINATE, FALSE, proc_id);
        if (process_handle && ::TerminateProcess(process_handle, 0))
        {
            result = ::WaitForSingleObject(process_handle, 10000) == WAIT_OBJECT_0;
        }
        else
        {
            //todo
            //error_handler();
        }

        if (process_handle) ::CloseHandle(process_handle);

        return result;
    }
    auto global_startup_dir()
    {
        auto result = text{};

        //todo vista & later
        // use SHGetFolderPath
        //PWSTR path;
        //if (S_OK != SHGetKnownFolderPath(FOLDERID_CommonStartup, 0, NULL, &path))
        //{
        //	//todo
        //	//error_handler();
        //}
        //else
        //{
        //	result = utils::to_utf(wide(path)) + '\\';
        //	CoTaskMemFree(path);
        //}

        auto pidl = PIDLIST_ABSOLUTE{};
        if (S_OK == ::SHGetFolderLocation(NULL, CSIDL_COMMON_STARTUP, NULL, 0, &pidl))
        {
            char path[MAX_PATH];
            if (TRUE == ::SHGetPathFromIDList(pidl, path))
            {
                result = text(path) + '\\';
            }
            ::ILFree(pidl);
        }
        else
        {
            //todo
            //error_handler();
        }

        return result;
    }
    auto check_pipe(text const& path, text prefix = "\\\\.\\pipe\\")
    {
        auto hits = faux;
        auto next = WIN32_FIND_DATAA{};
        auto name = path.substr(prefix.size());
        auto what = prefix + '*';
        auto hndl = ::FindFirstFileA(what.c_str(), &next);
        if (hndl != INVALID_FD)
        {
            do hits = next.cFileName == name;
            while (!hits && ::FindNextFileA(hndl, &next));

            if (hits) log("path: ", path);
            ::FindClose(hndl);
        }
        return hits;
    }

    #endif  // Windows specific

    struct fire
    {
        #if defined(_WIN32)

            fd_t h; // fire: Descriptor for IO interrupt.

            operator auto () { return h; }
            fire(bool i = 1) { ok(h = ::CreateEvent(NULL, i, FALSE, NULL), "CreateEvent error"); }
           ~fire()           { os::close(h); }
            void reset()     { ok(::SetEvent(h), "SetEvent error"); }
            void flush()     { ok(::ResetEvent(h), "ResetEvent error"); }

        #else

            fd_t h[2] = { INVALID_FD, INVALID_FD }; // fire: Descriptors for IO interrupt.
            char x = 1;

            operator auto () { return h[0]; }
            fire()           { ok(::pipe(h), "pipe[2] creation failed"); }
           ~fire()           { for (auto& f : h) os::close(f); }
            void reset()     { os::send(h[1], &x, sizeof(x)); }
            void flush()     { os::recv(h[0], &x, sizeof(x)); }

        #endif
        void bell() { reset(); }
    };

    class pool
    {
        struct item
        {
            bool        state;
            std::thread guest;
        };

        std::recursive_mutex            mutex;
        std::condition_variable_any     synch;
        std::map<std::thread::id, item> index;
        si32                            count;
        bool                            alive;
        std::thread                     agent;

        void worker()
        {
            auto guard = std::unique_lock{ mutex };
            log("pool: session control started");

            while (alive)
            {
                synch.wait(guard);
                for (auto it = index.begin(); it != index.end();)
                {
                    auto& [sid, session] = *it;
                    auto& [state, guest] = session;
                    if (state == faux || !alive)
                    {
                        if (guest.joinable())
                        {
                            guard.unlock();
                            guest.join();
                            guard.lock();
                            log("pool: id: ", sid, " session joined");
                        }
                        it = index.erase(it);
                    }
                    else ++it;
                }
            }
        }
        void checkout()
        {
            auto guard = std::lock_guard{ mutex };
            auto session_id = std::this_thread::get_id();
            index[session_id].state = faux;
            synch.notify_one();
            log("pool: id: ", session_id, " session deleted");
        }

    public:
        template<class PROC>
        void run(PROC process)
        {
            auto guard = std::lock_guard{ mutex };
            auto next_id = count++;
            auto session = std::thread([&, process, next_id]
            {
                process(next_id);
                checkout();
            });
            auto session_id = session.get_id();
            index[session_id] = { true, std::move(session) };
            log("pool: id: ", session_id, " session created");
        }
        auto size()
        {
            return index.size();
        }

        pool()
            : count{ 0    },
              alive{ true },
              agent{ &pool::worker, this }
        { }
       ~pool()
        {
            mutex.lock();
            alive = faux;
            synch.notify_one();
            mutex.unlock();

            if (agent.joinable())
            {
                log("pool: joining agent");
                agent.join();
            }
            log("pool: session control ended");
        }
    };

    namespace ipc
    {
        struct stdcon
        {
            using flux = std::ostream;

            bool active; // ipc::stdcon: Is connected.
            file handle; // ipc::stdcon: IO descriptor.
            text buffer; // ipc::stdcon: Receive buffer.

            stdcon()
                : active{ faux },
                  buffer(PIPE_BUF*100, 0)
            { }
            stdcon(file&& fd)
                : active{ true },
                  handle{ std::move(fd) },
                  buffer(PIPE_BUF*100, 0)
            { }
            stdcon(fd_t r, fd_t w)
                : active{ true },
                  handle{ r, w },
                  buffer(PIPE_BUF*100, 0)
            { }
            virtual ~stdcon()
            { }

            void operator = (stdcon&& p)
            {
                handle = std::move(p.handle);
                buffer = std::move(p.buffer);
                active = p.active;
                p.active = faux;
            }
            operator bool () { return active; }

            virtual bool send(view buff)
            {
                return os::send(handle.w, buff.data(), buff.size());
            }
            virtual qiew recv(char* buff, size_t size)
            {
                return os::recv(handle, buff, size); // The read call can be interrupted by the write side when its read call is interrupted.
            }
            virtual qiew recv() // It's not thread safe!
            {
                return recv(buffer.data(), buffer.size());
            }
            virtual void shut()
            {
                active = faux;
                handle.shutdown(); // Close the writing handle to interrupt a reading call on the server side and trigger to close the server writing handle to interrupt owr reading call.
            }
            virtual void stop()
            {
                shut();
            }
            virtual flux& show(flux& s) const
            {
                return s << handle;
            }
            friend auto& operator << (flux& s, ipc::stdcon const& sock)
            {
                return sock.show(s << "{ xipc: ") << " }";
            }
            friend auto& operator << (std::ostream& s, netxs::os::xipc const& sock)
            {
                return s << *sock;
            }
            void output(view data)
            {
                send(data);
            }
        };

        struct memory
            : public stdcon
        {
            class fifo
            {
                using lock = std::mutex;
                using sync = std::condition_variable;

                bool alive;
                text store;
                lock mutex;
                sync wsync;
                sync rsync;

            public:
                fifo()
                    : alive{ true }
                { }

                auto send(view block)
                {
                    auto guard = std::unique_lock{ mutex };
                    if (store.size() && alive) rsync.wait(guard, [&]{ return store.empty() || !alive; });
                    if (alive)
                    {
                        store = block;
                        wsync.notify_one();
                    }
                    return alive;
                }
                auto read(text& yield)
                {
                    auto guard = std::unique_lock{ mutex };
                    wsync.wait(guard, [&]{ return store.size() || !alive; });
                    if (alive)
                    {
                        std::swap(store, yield);
                        store.clear();
                        rsync.notify_one();
                        return qiew{ yield };
                    }
                    else return qiew{};
                }
                void stop()
                {
                    auto guard = std::lock_guard{ mutex };
                    alive = faux;
                    wsync.notify_one();
                    rsync.notify_one();
                }
            };

            sptr<fifo> server;
            sptr<fifo> client;

            memory(sptr<fifo> s_queue = std::make_shared<fifo>(),
                   sptr<fifo> c_queue = std::make_shared<fifo>())
                : server{ s_queue },
                  client{ c_queue }
            {
                active = true;
            }

            qiew recv() override
            {
                return server->read(buffer);
            }
            bool send(view data) override
            {
                return client->send(data);
            }
            flux& show(flux& s) const override
            {
                return s << "local pipe: server=" << server.get() << " client=" << client.get();
            }
            void shut() override
            {
                stdcon::shut();
                server->stop();
                client->stop();
            }
        };

        struct socket
            : public stdcon
        {
            text scpath; // ipc:socket: Socket path (in order to unlink).
            fire signal; // ipc:socket: Interruptor.

            socket(file& fd)
                : stdcon{ std::move(fd) }
            { }
            socket(fd_t r, fd_t w, text path)
                : stdcon{ r, w },
                  scpath{ path }
            { }
           ~socket()
            {
                #if defined(__BSD__)

                    if (scpath.length())
                    {
                        ::unlink(scpath.c_str()); // Cleanup file system unix domain socket.
                    }

                #endif
            }

            template<class T>
            auto cred(T id) const // Check peer cred.
            {
                #if defined(_WIN32)

                    //Note: Named Pipes - default ACL used for a named pipe grant full control to the LocalSystem, admins, and the creator owner
                    //https://docs.microsoft.com/en-us/windows/win32/ipc/named-pipe-security-and-access-rights

                #elif defined(__linux__)

                    auto cred = ucred{};
                    #ifndef __ANDROID__
                        auto size = socklen_t{ sizeof(cred) };
                    #else
                        auto size = unsigned{ sizeof(cred) };
                    #endif

                    if (!ok(::getsockopt(handle.r, SOL_SOCKET, SO_PEERCRED, &cred, &size), "getsockopt(SOL_SOCKET) failed"))
                    {
                        return faux;
                    }

                    if (cred.uid && id != cred.uid)
                    {
                        log("sock: other users are not allowed to the session, abort");
                        return faux;
                    }

                    log("sock: creds from SO_PEERCRED:",
                            "\n\t  pid: ", cred.pid,
                            "\n\t euid: ", cred.uid,
                            "\n\t egid: ", cred.gid);

                #elif defined(__BSD__)

                    auto euid = uid_t{};
                    auto egid = gid_t{};

                    if (!ok(::getpeereid(handle.r, &euid, &egid), "getpeereid failed"))
                    {
                        return faux;
                    }

                    if (euid && id != euid)
                    {
                        log("sock: other users are not allowed to the session, abort");
                        return faux;
                    }

                    log("sock: creds from getpeereid:",
                            "\n\t  pid: ", id,
                            "\n\t euid: ", euid,
                            "\n\t egid: ", egid);

                #endif

                return true;
            }
            auto meet()
            {
                auto client = sptr<ipc::socket>{};
                #if defined(_WIN32)

                    auto to_server = RD_PIPE_PATH + scpath;
                    auto to_client = WR_PIPE_PATH + scpath;
                    auto next_link = [&](auto h, auto const& path, auto type)
                    {
                        auto next_waiting_point = INVALID_FD;
                        auto connected = ::ConnectNamedPipe(h, NULL)
                            ? true
                            : (::GetLastError() == ERROR_PIPE_CONNECTED);

                        if (active && connected) // Recreate the waiting point for the next client.
                        {
                            next_waiting_point =
                                ::CreateNamedPipe(path.c_str(),             // pipe path
                                                  type,                     // read/write access
                                                  PIPE_TYPE_BYTE |          // message type pipe
                                                  PIPE_READMODE_BYTE |      // message-read mode
                                                  PIPE_WAIT,                // blocking mode
                                                  PIPE_UNLIMITED_INSTANCES, // max. instances
                                                  PIPE_BUF,                 // output buffer size
                                                  PIPE_BUF,                 // input buffer size
                                                  0,                        // client time-out
                                                  NULL);                    // DACL (pipe_acl)
                            // DACL: auto pipe_acl = security_descriptor(security_descriptor_string);
                            //       The ACLs in the default security descriptor for a named pipe grant full control to the
                            //       LocalSystem account, administrators, and the creator owner. They also grant read access to
                            //       members of the Everyone group and the anonymous account.
                            //       Without write access, the desktop will be inaccessible to non-owners.
                        }
                        else if (active) os::fail("meet: not active");

                        return next_waiting_point;
                    };

                    auto r = next_link(handle.r, to_server, PIPE_ACCESS_INBOUND);
                    if (r == INVALID_FD)
                    {
                        if (active) os::fail("meet: ::CreateNamedPipe unexpected (read)");
                    }
                    else
                    {
                        auto w = next_link(handle.w, to_client, PIPE_ACCESS_OUTBOUND);
                        if (w == INVALID_FD)
                        {
                            ::CloseHandle(r);
                            if (active) os::fail("meet: ::CreateNamedPipe unexpected (write)");
                        }
                        else
                        {
                            client = std::make_shared<ipc::socket>(handle);
                            handle = { r, w };
                        }
                    }

                #else

                    auto h_proc = [&]
                    {
                        auto h = ::accept(handle.r, 0, 0);
                        auto s = file{ h, h };
                        if (s) client = std::make_shared<ipc::socket>(s);
                    };
                    auto f_proc = [&]
                    {
                        log("meet: signal fired");
                        signal.flush();
                    };
                    os::select(handle.r, h_proc,
                               signal  , f_proc);

                #endif
                return client;
            }
            bool send(view buff) override
            {
                return os::send<faux>(handle.w, buff.data(), buff.size());
            }
            void stop() override
            {
                stdcon::shut();
                #if defined(_WIN32)

                    // Interrupt ::ConnectNamedPipe(). Disconnection order does matter.
                    auto to_client = WR_PIPE_PATH + scpath;
                    auto to_server = RD_PIPE_PATH + scpath;
                    // This may fail, but this is ok - it means the client is already disconnected.
                    if (handle.w != INVALID_FD) ::DeleteFileA(to_client.c_str());
                    if (handle.r != INVALID_FD) ::DeleteFileA(to_server.c_str());
                    handle.close();

                #else

                    signal.reset();

                #endif
            }
            void shut() override
            {
                stdcon::shut();
                #if defined(_WIN32)

                    // Disconnection order does matter.
                    // This may fail, but this is ok - it means the client is already disconnected.
                    if (handle.w != INVALID_FD) ::DisconnectNamedPipe(handle.w);
                    if (handle.r != INVALID_FD) ::DisconnectNamedPipe(handle.r);

                #else

                    //an important conceptual reason to want
                    //to use shutdown:
                    //             to signal EOF to the peer
                    //             and still be able to receive
                    //             pending data the peer sent.
                    //"shutdown() doesn't actually close the file descriptor
                    //            — it just changes its usability.
                    //To free a socket descriptor, you need to use close()."

                    log("xipc: shutdown: handle descriptor: ", handle.r);
                    if (!ok(::shutdown(handle.r, SHUT_RDWR), "descriptor shutdown error"))  // Further sends and receives are disallowed.
                    {
                        switch (errno)
                        {
                            case EBADF:    os::fail("EBADF: The socket argument is not a valid file descriptor.");                             break;
                            case EINVAL:   os::fail("EINVAL: The how argument is invalid.");                                                   break;
                            case ENOTCONN: os::fail("ENOTCONN: The socket is not connected.");                                                 break;
                            case ENOTSOCK: os::fail("ENOTSOCK: The socket argument does not refer to a socket.");                              break;
                            case ENOBUFS:  os::fail("ENOBUFS: Insufficient resources were available in the system to perform the operation."); break;
                            default:       os::fail("unknown reason");                                                                         break;
                        }
                    }

                #endif
            }
            template<role ROLE, class P = noop>
            static auto open(text path, datetime::period retry_timeout = {}, P retry_proc = P())
            {
                auto r = INVALID_FD;
                auto w = INVALID_FD;
                auto socket = sptr<ipc::socket>{};
                auto try_start = [&](auto play) -> bool
                {
                    auto done = play();
                    if (!done)
                    {
                        if constexpr (ROLE == role::client)
                        {
                            if (!retry_proc())
                            {
                                return os::fail("failed to start server");
                            }
                        }

                        auto stop = datetime::tempus::now() + retry_timeout;
                        do
                        {
                            std::this_thread::sleep_for(100ms);
                            done = play();
                        }
                        while (!done && stop > datetime::tempus::now());
                    }
                    return done;
                };

                #if defined(_WIN32)

                    //security_descriptor pipe_acl(security_descriptor_string);
                    //log("pipe: DACL=", pipe_acl.security_string);

                    auto to_server = RD_PIPE_PATH + path;
                    auto to_client = WR_PIPE_PATH + path;

                    if constexpr (ROLE == role::server)
                    {
                        if (os::check_pipe(to_server))
                        {
                            os::fail("server already running");
                            return socket;
                        }

                        auto pipe = [](auto const& path, auto type)
                        {
                            return ::CreateNamedPipe(path.c_str(),             // pipe path
                                                     type,                     // read/write access
                                                     PIPE_TYPE_BYTE |          // message type pipe
                                                     PIPE_READMODE_BYTE |      // message-read mode
                                                     PIPE_WAIT,                // blocking mode
                                                     PIPE_UNLIMITED_INSTANCES, // max instances
                                                     PIPE_BUF,                 // output buffer size
                                                     PIPE_BUF,                 // input buffer size
                                                     0,                        // client time-out
                                                     NULL);                    // DACL
                        };

                        r = pipe(to_server, PIPE_ACCESS_INBOUND);
                        if (r == INVALID_FD)
                        {
                            os::fail("::CreateNamedPipe unexpected (read)");
                        }
                        else
                        {
                            w = pipe(to_client, PIPE_ACCESS_OUTBOUND);
                            if (w == INVALID_FD)
                            {
                                os::fail("::CreateNamedPipe unexpected (write)");
                                os::close(r);
                            }
                            else
                            {
                                DWORD inpmode = 0;
                                ok(::GetConsoleMode(STDIN_FD , &inpmode), "::GetConsoleMode(STDIN_FD) unexpected");
                                inpmode |= 0
                                        | nt::console::inmode::quickedit
                                        ;
                                ok(::SetConsoleMode(STDIN_FD, inpmode), "::SetConsoleMode(STDIN_FD) unexpected");

                                DWORD outmode = 0
                                            | nt::console::outmode::preprocess
                                            | nt::console::outmode::vt
                                            | nt::console::outmode::no_auto_cr
                                            ;
                                ok(::SetConsoleMode(STDOUT_FD, outmode), "::SetConsoleMode(STDOUT_FD) unexpected");
                            }
                        }
                    }
                    else if constexpr (ROLE == role::client)
                    {
                        auto pipe = [](auto const& path, auto type)
                        {
                            return ::CreateFile(path.c_str(),  // pipe path
                                                type,
                                                0,             // no sharing
                                                NULL,          // default security attributes
                                                OPEN_EXISTING, // opens existing pipe
                                                0,             // default attributes
                                                NULL);         // no template file
                        };
                        auto play = [&]
                        {
                            w = pipe(to_server, GENERIC_WRITE);
                            if (w == INVALID_FD)
                            {
                                return faux;
                            }

                            r = pipe(to_client, GENERIC_READ);
                            if (r == INVALID_FD)
                            {
                                os::close(w);
                                return faux;
                            }
                            return true;
                        };
                        if (!try_start(play))
                        {
                            os::fail("connection error");
                        }
                    }

                #else

                    ok(::signal(SIGPIPE, SIG_IGN), "failed to set SIG_IGN");

                    auto addr = sockaddr_un{};
                    auto sun_path = addr.sun_path + 1; // Abstract namespace socket (begins with zero). The abstract socket namespace is a nonportable Linux extension.

                    #if defined(__BSD__)
                        //todo unify "/.config/vtm"
                        auto home = os::homepath() / ".config/vtm";
                        if (!fs::exists(home))
                        {
                            log("path: create home directory '", home.string(), "'");
                            auto ec = std::error_code{};
                            fs::create_directory(home, ec);
                            if (ec) log("path: directory '", home.string(), "' creation error ", ec.value());
                        }
                        path = (home / path).string() + ".sock";
                        sun_path--; // File system unix domain socket.
                        log("open: file system socket ", path);
                    #endif

                    if (path.size() > sizeof(sockaddr_un::sun_path) - 2)
                    {
                        os::fail("socket path too long");
                    }
                    else if ((w = ::socket(AF_UNIX, SOCK_STREAM, 0)) == INVALID_FD)
                    {
                        os::fail("open unix domain socket error");
                    }
                    else
                    {
                        r = w;
                        addr.sun_family = AF_UNIX;
                        auto sock_addr_len = (socklen_t)(sizeof(addr) - (sizeof(sockaddr_un::sun_path) - path.size() - 1));
                        std::copy(path.begin(), path.end(), sun_path);

                        auto play = [&]
                        {
                            return -1 != ::connect(r, (struct sockaddr*)&addr, sock_addr_len);
                        };

                        if constexpr (ROLE == role::server)
                        {
                            #if defined(__BSD__)
                                if (fs::exists(path))
                                {
                                    if (play())
                                    {
                                        os::fail("server already running");
                                        os::close(r);
                                    }
                                    else
                                    {
                                        log("path: remove file system socket file ", path);
                                        ::unlink(path.c_str()); // Cleanup file system socket.
                                    }
                                }
                            #endif

                            if (r != INVALID_FD && ::bind(r, (struct sockaddr*)&addr, sock_addr_len) == -1)
                            {
                                os::fail("error unix socket bind for ", path);
                                os::close(r);
                            }
                            else if (::listen(r, 5) == -1)
                            {
                                os::fail("error listen socket for ", path);
                                os::close(r);
                            }
                        }
                        else if constexpr (ROLE == role::client)
                        {
                            path.clear(); // No need to unlink a file system socket on client disconnect.
                            if (!try_start(play))
                            {
                                os::fail("connection error");
                                os::close(r);
                            }
                        }
                    }

                #endif
                if (r != INVALID_FD && w != INVALID_FD)
                {
                    socket = std::make_shared<ipc::socket>(r, w, path);
                }
                return socket;
            }
        };

        auto stdio()
        {
            return std::make_shared<ipc::stdcon>(STDIN_FD, STDOUT_FD);
        }
        auto xlink()
        {
            auto a = std::make_shared<ipc::memory>();
            auto b = std::make_shared<ipc::memory>(a->client, a->server); // Swap queues for xlink.
            return std::pair{ a, b };
        }
    }

    namespace tty
    {
        auto& globals()
        {
            struct
            {
                xipc                     ipcio; // globals: STDIN/OUT.
                conmode                  state; // globals: Saved console mode to restore at exit.
                testy<twod>              winsz; // globals: Current console window size.
                ansi::dtvt::binary::s11n wired; // globals: Serialization buffers.
                si32                     kbmod; // globals: Keyboard modifiers state.
                fire                     alarm; // globals: IO interrupter.
            }
            static vars;
            return vars;
        }
        void resize()
        {
            static constexpr auto winsz_fallback = twod{ 132, 60 };
            auto& g = globals();
            auto& ipcio =*g.ipcio;
            auto& winsz = g.winsz;
            auto& wired = g.wired;

            #if defined(_WIN32)

                auto cinfo = CONSOLE_SCREEN_BUFFER_INFO{};
                if (ok(::GetConsoleScreenBufferInfo(STDOUT_FD, &cinfo), "GetConsoleScreenBufferInfo failed"))
                {
                    winsz({ cinfo.srWindow.Right  - cinfo.srWindow.Left + 1,
                            cinfo.srWindow.Bottom - cinfo.srWindow.Top  + 1 });
                }

            #else

                auto size = winsize{};
                if (ok(::ioctl(STDOUT_FD, TIOCGWINSZ, &size), "ioctl(STDOUT_FD, TIOCGWINSZ) failed"))
                {
                    winsz({ size.ws_col, size.ws_row });
                }

            #endif

            if (winsz == dot_00)
            {
                log("xtty: fallback tty window size ", winsz_fallback, " (consider using 'ssh -tt ...')");
                winsz(winsz_fallback);
            }
            wired.winsz.send(ipcio, 0, winsz.last);
        }
        void repair()
        {
            auto& state = globals().state;
            #if defined(_WIN32)
                ok(::SetConsoleMode(STDOUT_FD, state[0]), "SetConsoleMode failed (revert_o)");
                ok(::SetConsoleMode(STDIN_FD , state[1]), "SetConsoleMode failed (revert_i)");
            #else
                ::tcsetattr(STDIN_FD, TCSANOW, &state);
            #endif
        }
        auto signal(sigB what)
        {
            #if defined(_WIN32)

                auto& g = globals();
                switch (what)
                {
                    case CTRL_C_EVENT:
                    {
                        /* placed to the input buffer - ENABLE_PROCESSED_INPUT is disabled */
                        /* never happen */
                        break;
                    }
                    case CTRL_BREAK_EVENT:
                    {
                        auto dwControlKeyState = g.kbmod;
                        auto wVirtualKeyCode  = ansi::ctrl_break;
                        auto wVirtualScanCode = ansi::ctrl_break;
                        auto bKeyDown = faux;
                        auto wRepeatCount = 1;
                        auto UnicodeChar = L'\x03'; // ansi::C0_ETX
                        g.wired.syskeybd.send(*g.ipcio,
                            0,
                            os::kbstate(g.kbmod, dwControlKeyState),
                            dwControlKeyState,
                            wVirtualKeyCode,
                            wVirtualScanCode,
                            bKeyDown,
                            wRepeatCount,
                            UnicodeChar ? utf::to_utf(UnicodeChar) : text{},
                            UnicodeChar);
                        break;
                    }
                    case CTRL_CLOSE_EVENT:
                        /* do nothing */
                        break;
                    case CTRL_LOGOFF_EVENT:
                        /* todo signal global */
                        break;
                    case CTRL_SHUTDOWN_EVENT:
                        /* todo signal global */
                        break;
                    default:
                        break;
                }
                return TRUE;

            #else

                auto shutdown = [](auto what)
                {
                    globals().ipcio->stop();
                    ::signal(what, SIG_DFL);
                    ::raise(what);
                };
                switch (what)
                {
                    case SIGWINCH: resize(); return;
                    case SIGHUP:   log(" tty: SIGHUP");  shutdown(what); break;
                    case SIGTERM:  log(" tty: SIGTERM"); shutdown(what); break;
                    default:       log(" tty: signal ", what); break;
                }

            #endif
        }
        auto logger(si32 mode)
        {
            auto direct = !!(mode & os::legacy::direct);
            return direct ? netxs::logger([](auto data) { os::stdlog(data); })
                          : netxs::logger([](auto data) { os::syslog(data); });
        }
        void direct(xipc link)
        {
            auto cout = os::ipc::stdio();
            auto& extio = *cout;
            auto& ipcio = *link;
            auto input = std::thread{ [&]
            {
                while (extio && extio.send(ipcio.recv())) { }
                extio.shut();
            }};
            while (ipcio && ipcio.send(extio.recv())) { }
            ipcio.shut();
            input.join();
        }
        void reader(si32 mode)
        {
            log(" tty: id: ", std::this_thread::get_id(), " reading thread started");
            auto& g = globals();
            auto& ipcio =*g.ipcio;
            auto& wired = g.wired;
            auto& alarm = g.alarm;

            #if defined(_WIN32)

                // The input codepage to UTF-8 is severely broken in all Windows versions.
                // ReadFile and ReadConsoleA either replace non-ASCII characters with NUL
                // or return 0 bytes read.
                auto reply = std::vector<INPUT_RECORD>(1);
                auto count = DWORD{};
                auto stamp = ui32{};
                fd_t waits[] = { STDIN_FD, alarm };
                while (WAIT_OBJECT_0 == ::WaitForMultipleObjects(2, waits, FALSE, INFINITE))
                {
                    if (!::GetNumberOfConsoleInputEvents(STDIN_FD, &count))
                    {
                        // ERROR_PIPE_NOT_CONNECTED
                        // 233 (0xE9)
                        // No process is on the other end of the pipe.
                        //defeat("GetNumberOfConsoleInputEvents error");
                        os::exit(-1, " tty: GetNumberOfConsoleInputEvents error ", ::GetLastError());
                        break;
                    }
                    else if (count)
                    {
                        if (count > reply.size()) reply.resize(count);

                        if (!::ReadConsoleInputW(STDIN_FD, reply.data(), (DWORD)reply.size(), &count))
                        {
                            //ERROR_PIPE_NOT_CONNECTED = 0xE9 - it's means that the console is gone/crashed
                            //defeat("ReadConsoleInput error");
                            os::exit(-1, " tty: ReadConsoleInput error ", ::GetLastError());
                            break;
                        }
                        else
                        {
                            auto entry = reply.begin();
                            auto limit = entry + count;
                            while (entry != limit)
                            {
                                auto& reply = *entry++;
                                switch (reply.EventType)
                                {
                                    case KEY_EVENT:
                                        wired.syskeybd.send(ipcio,
                                            0,
                                            os::kbstate(g.kbmod, reply.Event.KeyEvent.dwControlKeyState, reply.Event.KeyEvent.wVirtualScanCode, reply.Event.KeyEvent.bKeyDown),
                                            reply.Event.KeyEvent.dwControlKeyState,
                                            reply.Event.KeyEvent.wVirtualKeyCode,
                                            reply.Event.KeyEvent.wVirtualScanCode,
                                            reply.Event.KeyEvent.bKeyDown,
                                            reply.Event.KeyEvent.wRepeatCount,
                                            reply.Event.KeyEvent.uChar.UnicodeChar ? utf::to_utf(reply.Event.KeyEvent.uChar.UnicodeChar) : text{},
                                            reply.Event.KeyEvent.uChar.UnicodeChar);
                                        break;
                                    case MOUSE_EVENT:
                                        wired.sysmouse.send(ipcio,
                                            0,
                                            input::hids::ok,
                                            os::kbstate(g.kbmod, reply.Event.MouseEvent.dwControlKeyState),
                                            reply.Event.MouseEvent.dwControlKeyState,
                                            reply.Event.MouseEvent.dwButtonState & 0b00011111,
                                            reply.Event.MouseEvent.dwEventFlags & DOUBLE_CLICK,
                                            reply.Event.MouseEvent.dwEventFlags & MOUSE_WHEELED,
                                            reply.Event.MouseEvent.dwEventFlags & MOUSE_HWHEELED,
                                            static_cast<int16_t>((0xFFFF0000 & reply.Event.MouseEvent.dwButtonState) >> 16), // dwButtonState too large when mouse scrolls
                                            twod{ reply.Event.MouseEvent.dwMousePosition.X, reply.Event.MouseEvent.dwMousePosition.Y },
                                            stamp++);
                                        break;
                                    case WINDOW_BUFFER_SIZE_EVENT:
                                        resize();
                                        break;
                                    case FOCUS_EVENT:
                                        wired.sysfocus.send(ipcio,
                                            0,
                                            reply.Event.FocusEvent.bSetFocus,
                                            faux,
                                            faux);
                                        break;
                                    default:
                                        break;
                                }
                            }
                        }
                    }
                }

            #else

                auto legacy_mouse = !!(mode & os::legacy::mouse);
                auto legacy_color = !!(mode & os::legacy::vga16);
                auto micefd = INVALID_FD;
                auto mcoord = twod{};
                auto buffer = text(os::stdin_buf_size, '\0');
                auto ttynum = si32{ 0 };

                auto get_kb_state = []
                {
                    auto state = si32{ 0 };
                    #if defined(__linux__)
                        auto shift_state = si32{ 6 };
                        ok(::ioctl(STDIN_FD, TIOCLINUX, &shift_state), "ioctl(STDIN_FD, TIOCLINUX) failed");
                        state = 0
                            | (shift_state & (1 << KG_ALTGR)) >> 1 // 0x1
                            | (shift_state & (1 << KG_ALT  )) >> 2 // 0x2
                            | (shift_state & (1 << KG_CTRLR)) >> 5 // 0x4
                            | (shift_state & (1 << KG_CTRL )) << 1 // 0x8
                            | (shift_state & (1 << KG_SHIFT)) << 4 // 0x10
                            ;
                    #endif
                    return state;
                };
                ok(::ttyname_r(STDOUT_FD, buffer.data(), buffer.size()), "ttyname_r(STDOUT_FD) failed");
                auto tty_name = view(buffer.data());
                log(" tty: pseudoterminal ", tty_name);
                if (legacy_mouse)
                {
                    log(" tty: compatibility mode");
                    auto imps2_init_string = "\xf3\xc8\xf3\x64\xf3\x50";
                    auto mouse_device = "/dev/input/mice";
                    auto mouse_fallback1 = "/dev/input/mice.vtm";
                    auto mouse_fallback2 = "/dev/input/mice_vtm"; //todo deprecated
                    auto fd = ::open(mouse_device, O_RDWR);
                    if (fd == -1) fd = ::open(mouse_fallback1, O_RDWR);
                    if (fd == -1) log(" tty: error opening ", mouse_device, " and ", mouse_fallback1, ", error ", errno, errno == 13 ? " - permission denied" : "");
                    if (fd == -1) fd = ::open(mouse_fallback2, O_RDWR);
                    if (fd == -1) log(" tty: error opening ", mouse_device, " and ", mouse_fallback2, ", error ", errno, errno == 13 ? " - permission denied" : "");
                    else if (os::send(fd, imps2_init_string, sizeof(imps2_init_string)))
                    {
                        char ack;
                        os::recv(fd, &ack, sizeof(ack));
                        micefd = fd;
                        auto tty_word = tty_name.find("tty", 0);
                        if (tty_word != text::npos)
                        {
                            tty_word += 3; /*skip tty letters*/
                            auto tty_number = utf::to_view(buffer.data() + tty_word, buffer.size() - tty_word);
                            if (auto cur_tty = utf::to_int(tty_number))
                            {
                                ttynum = cur_tty.value();
                            }
                        }
                        wired.mouse_show.send(ipcio, true);
                        if (ack == '\xfa') log(" tty: ImPS/2 mouse connected, fd: ", fd);
                        else               log(" tty: unknown PS/2 mouse connected, fd: ", fd, " ack: ", (int)ack);
                    }
                    else
                    {
                        log(" tty: mouse initialization error");
                        os::close(fd);
                    }
                }

                auto m = input::sysmouse{};
                auto k = input::syskeybd{};
                auto f = input::sysfocus{};
                auto close = faux;
                auto total = text{};
                auto digit = [](auto c) { return c >= '0' && c <= '9'; };

                // The following sequences are processed here:
                // ESC
                // ESC ESC
                // ESC [ I
                // ESC [ O
                // ESC [ < 0 ; x ; y M/m
                auto filter = [&](view accum)
                {
                    total += accum;
                    auto strv = view{ total };

                    //#if defined(KEYLOG)
                    //    log("link: input data (", total.size(), " bytes):\n", utf::debase(total));
                    //#endif

                    //#ifndef PROD
                    //if (close)
                    //{
                    //    close = faux;
                    //    notify(e2::conio::preclose, close);
                    //    if (total.front() == '\033') // two consecutive escapes
                    //    {
                    //        log("\t - two consecutive escapes: \n\tstrv:        ", strv);
                    //        notify(e2::conio::quit, "pipe two consecutive escapes");
                    //        return;
                    //    }
                    //}
                    //#endif

                    //todo unify (it is just a proof of concept)
                    while (auto len = strv.size())
                    {
                        auto pos = 0_sz;
                        auto unk = faux;

                        if (strv.at(0) == '\033')
                        {
                            ++pos;

                            //#ifndef PROD
                            //if (pos == len) // the only one esc
                            //{
                            //    close = true;
                            //    total = strv;
                            //    log("\t - preclose: ", canal);
                            //    notify(e2::conio::preclose, close);
                            //    break;
                            //}
                            //else if (strv.at(pos) == '\033') // two consecutive escapes
                            //{
                            //    total.clear();
                            //    log("\t - two consecutive escapes: ", canal);
                            //    notify(e2::conio::quit, "pipe2: two consecutive escapes");
                            //    break;
                            //}
                            //#else
                            if (pos == len) // the only one esc
                            {
                                // Pass Esc.
                                k.gear_id = 0;
                                k.pressed = true;
                                k.cluster = strv.substr(0, 1);
                                wired.syskeybd.send(ipcio, k);
                                total.clear();
                                break;
                            }
                            else if (strv.at(pos) == '\033') // two consecutive escapes
                            {
                                // Pass Esc.
                                k.gear_id = 0;
                                k.pressed = true;
                                k.cluster = strv.substr(0, 1);
                                wired.syskeybd.send(ipcio, k);
                                total = strv.substr(1);
                                break;
                            }
                            //#endif
                            else if (strv.at(pos) == '[')
                            {
                                if (++pos == len) { total = strv; break; }//incomlpete
                                if (strv.at(pos) == 'I')
                                {
                                    f.gear_id = 0;
                                    f.enabled = true;
                                    wired.sysfocus.send(ipcio, f);
                                    ++pos;
                                }
                                else if (strv.at(pos) == 'O')
                                {
                                    f.gear_id = 0;
                                    f.enabled = faux;
                                    wired.sysfocus.send(ipcio, f);
                                    ++pos;
                                }
                                else if (strv.at(pos) == '<') // \033[<0;x;yM/m
                                {
                                    if (++pos == len) { total = strv; break; }// incomlpete sequence

                                    auto tmp = strv.substr(pos);
                                    auto l = tmp.size();
                                    if (auto ctrl = utf::to_int(tmp))
                                    {
                                        pos += l - tmp.size();
                                        if (pos == len) { total = strv; break; }// incomlpete sequence
                                        {
                                            if (++pos == len) { total = strv; break; }// incomlpete sequence

                                            auto tmp = strv.substr(pos);
                                            auto l = tmp.size();
                                            if (auto pos_x = utf::to_int(tmp))
                                            {
                                                pos += l - tmp.size();
                                                if (pos == len) { total = strv; break; }// incomlpete sequence
                                                {
                                                    if (++pos == len) { total = strv; break; }// incomlpete sequence

                                                    auto tmp = strv.substr(pos);
                                                    auto l = tmp.size();
                                                    if (auto pos_y = utf::to_int(tmp))
                                                    {
                                                        pos += l - tmp.size();
                                                        if (pos == len) { total = strv; break; }// incomlpete sequence
                                                        {
                                                            auto ispressed = (strv.at(pos) == 'M');
                                                            ++pos;

                                                            auto clamp = [](auto a) { return std::clamp(a,
                                                                std::numeric_limits<si32>::min() / 2,
                                                                std::numeric_limits<si32>::max() / 2); };

                                                            auto x = clamp(pos_x.value() - 1);
                                                            auto y = clamp(pos_y.value() - 1);
                                                            auto ctl = ctrl.value();

                                                            m.gear_id = {};
                                                            m.enabled = {};
                                                            m.winctrl = {};
                                                            m.doubled = {};
                                                            m.wheeled = {};
                                                            m.hzwheel = {};
                                                            m.wheeldt = {};
                                                            m.ctlstat = {};
                                                            // 000 000 00
                                                            //   | ||| ||
                                                            //   | ||| ------ button number
                                                            //   | ---------- ctl state
                                                            if (ctl & 0x04) m.ctlstat |= input::hids::LShift;
                                                            if (ctl & 0x08) m.ctlstat |= input::hids::LAlt;
                                                            if (ctl & 0x10) m.ctlstat |= input::hids::LCtrl;
                                                            ctl &= ~0b00011100;

                                                            if (ctl == 35 && m.buttons) // Moving without buttons (case when second release not fired: apple's terminal.app)
                                                            {
                                                                m.buttons = {};
                                                                m.changed++;
                                                                wired.sysmouse.send(ipcio, m);
                                                            }
                                                            m.coordxy = { x, y };
                                                            switch (ctl)
                                                            {
                                                                case 0: netxs::set_bit<input::hids::left  >(m.buttons, ispressed); break;
                                                                case 1: netxs::set_bit<input::hids::middle>(m.buttons, ispressed); break;
                                                                case 2: netxs::set_bit<input::hids::right >(m.buttons, ispressed); break;
                                                                case 64:
                                                                    m.wheeled = true;
                                                                    m.wheeldt = 1;
                                                                    break;
                                                                case 65:
                                                                    m.wheeled = true;
                                                                    m.wheeldt = -1;
                                                                    break;
                                                            }
                                                            m.changed++;
                                                            wired.sysmouse.send(ipcio, m);
                                                        }
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                                else
                                {
                                    unk = true;
                                    pos = 0_sz;
                                }
                            }
                            else
                            {
                                unk = true;
                                pos = 0_sz;
                            }
                        }

                        if (!unk)
                        {
                            total = strv.substr(pos);
                            strv = total;
                        }

                        if (auto size = strv.size())
                        {
                            auto i = unk ? 1_sz : 0_sz;
                            while (i != size && (strv.at(i) != '\033'))
                            {
                                // Pass SIGINT inside the desktop
                                //if (strv.at(i) == 3 /*3 - SIGINT*/)
                                //{
                                //	log(" - SIGINT in stdin");
                                //	owner.SIGNAL(tier::release, e2::conio::quit, "pipe: SIGINT");
                                //	return;
                                //}
                                i++;
                            }

                            if (i)
                            {
                                k.gear_id = 0;
                                k.pressed = true;
                                k.cluster = strv.substr(0, i);
                                wired.syskeybd.send(ipcio, k);
                                total = strv.substr(i);
                                strv = total;
                            }
                        }
                    }
                };

                auto h_proc = [&]
                {
                    auto data = os::recv(STDIN_FD, buffer.data(), buffer.size());
                    if (micefd != INVALID_FD)
                    {
                        auto kb_state = get_kb_state();
                        if (m.ctlstat != kb_state)
                        {
                            m.ctlstat = kb_state;
                            wired.ctrls.send(ipcio,
                                m.gear_id,
                                m.ctlstat);
                        }
                     }
                    filter(data);
                };
                auto m_proc = [&]
                {
                    auto data = os::recv(micefd, buffer.data(), buffer.size());
                    auto size = data.size();
                    if (size == 4 /* ImPS/2 */
                     || size == 3 /* PS/2 compatibility mode */)
                    {
                    #if defined(__linux__)
                        auto vt_state = vt_stat{};
                        ok(::ioctl(STDOUT_FD, VT_GETSTATE, &vt_state), "ioctl(VT_GETSTATE) failed");
                        if (vt_state.v_active == ttynum) // Proceed current active tty only.
                        {
                            auto scale = twod{ 6,12 }; //todo magic numbers
                            auto limit = g.winsz.last * scale;
                            auto bttns = data[0] & 7;
                            mcoord.x  += data[1];
                            mcoord.y  -= data[2];
                            if (bttns == 0) mcoord = std::clamp(mcoord, dot_00, limit);
                            m.wheeldt = size == 4 ? -data[3] : 0;
                            m.wheeled = m.wheeldt;
                            m.coordxy = { mcoord / scale };
                            m.buttons = bttns;
                            m.ctlstat = get_kb_state();
                            m.changed++;
                            wired.sysmouse.send(ipcio, m);
                        }
                    #endif
                    }
                };
                auto f_proc = [&]
                {
                    alarm.flush();
                };

                while (ipcio)
                {
                    if (micefd != INVALID_FD)
                    {
                        os::select(STDIN_FD, h_proc,
                                   micefd,   m_proc,
                                   alarm,    f_proc);
                    }
                    else
                    {
                        os::select(STDIN_FD, h_proc,
                                   alarm,    f_proc);
                    }
                }

                os::close(micefd);

            #endif

            log(" tty: id: ", std::this_thread::get_id(), " reading thread ended");
        }
        void clipbd(si32 mode)
        {
            if (mode & legacy::direct) return;
            log(" tty: id: ", std::this_thread::get_id(), " clipboard watcher thread started");

            #if defined(_WIN32)

                auto wndname = text{ "vtmWindowClass" };
                auto wndproc = [](auto hwnd, auto uMsg, auto wParam, auto lParam)
                {
                    auto& g = globals();
                    auto& ipcio =*g.ipcio;
                    auto& wired = g.wired;
                    auto gear_id = id_t{ 0 };
                    switch (uMsg)
                    {
                        case WM_CREATE:
                            ok(::AddClipboardFormatListener(hwnd), "unexpected result from ::AddClipboardFormatListener()");
                        case WM_CLIPBOARDUPDATE:
                        {
                            auto lock = std::lock_guard{ os::clipboard_mutex };
                            while (!::OpenClipboard(hwnd)) // Waiting clipboard access.
                            {
                                if (os::error() != ERROR_ACCESS_DENIED)
                                {
                                    auto error = "OpenClipboard() returns unexpected result, code "s + std::to_string(os::error());
                                    wired.osclipdata.send(ipcio, gear_id, error, ansi::clip::textonly);
                                    return (LRESULT) NULL;
                                }
                                std::this_thread::yield();
                            }
                            if (auto seqno = ::GetClipboardSequenceNumber();
                                     seqno != os::clipboard_sequence)
                            {
                                os::clipboard_sequence = seqno;
                                if (auto format = ::EnumClipboardFormats(0))
                                {
                                    auto hidden = ::GetClipboardData(cf_sec1);
                                    if (auto hglb = ::GetClipboardData(cf_ansi)) // Our clipboard format.
                                    {
                                        if (auto lptr = ::GlobalLock(hglb))
                                        {
                                            auto size = ::GlobalSize(hglb);
                                            auto data = view((char*)lptr, size - 1/*trailing null*/);
                                            auto mime = ansi::clip::disabled;
                                            wired.osclipdata.send(ipcio, gear_id, text{ data }, mime);
                                            ::GlobalUnlock(hglb);
                                        }
                                        else
                                        {
                                            auto error = "GlobalLock() returns unexpected result, code "s + std::to_string(os::error());
                                            wired.osclipdata.send(ipcio, gear_id, error, ansi::clip::textonly);
                                        }
                                    }
                                    else do
                                    {
                                        if (format == os::cf_text)
                                        {
                                            auto mime = hidden ? ansi::clip::safetext : ansi::clip::textonly;
                                            if (auto hglb = ::GetClipboardData(format))
                                            if (auto lptr = ::GlobalLock(hglb))
                                            {
                                                auto size = ::GlobalSize(hglb);
                                                wired.osclipdata.send(ipcio, gear_id, utf::to_utf((wchr*)lptr, size / 2 - 1/*trailing null*/), mime);
                                                ::GlobalUnlock(hglb);
                                                break;
                                            }
                                            auto error = "GlobalLock() returns unexpected result, code "s + std::to_string(os::error());
                                            wired.osclipdata.send(ipcio, gear_id, error, ansi::clip::textonly);
                                        }
                                        else
                                        {
                                            //todo proceed other formats (rich/html/...)
                                        }
                                    }
                                    while (format = ::EnumClipboardFormats(format));
                                }
                                else wired.osclipdata.send(ipcio, gear_id, text{}, ansi::clip::textonly);
                            }
                            ok(::CloseClipboard(), "::CloseClipboard");
                            break;
                        }
                        case WM_DESTROY:
                            ok(::RemoveClipboardFormatListener(hwnd), "unexpected result from ::RemoveClipboardFormatListener()");
                            ::PostQuitMessage(0);
                            break;
                        default: return DefWindowProc(hwnd, uMsg, wParam, lParam);
                    }
                    return (LRESULT) NULL;
                };
                auto wnddata = WNDCLASSEXA
                {
                    .cbSize        = sizeof(WNDCLASSEX),
                    .lpfnWndProc   = wndproc,
                    .lpszClassName = wndname.c_str(),
                };
                if (ok(::RegisterClassExA(&wnddata) || os::error() == ERROR_CLASS_ALREADY_EXISTS, "unexpected result from ::RegisterClassExA()"))
                {
                    auto& alarm = globals().alarm;
                    auto hwnd = ::CreateWindowExA(0, wndname.c_str(), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
                    auto next = MSG{};
                    while (next.message != WM_QUIT)
                    {
                        if (auto yield = ::MsgWaitForMultipleObjects(1, (fd_t*)&alarm, FALSE, INFINITE, QS_ALLINPUT);
                                 yield == WAIT_OBJECT_0)
                        {
                            ::DestroyWindow(hwnd);
                            break;
                        }
                        while (::PeekMessageA(&next, NULL, 0, 0, PM_REMOVE) && next.message != WM_QUIT)
                        {
                            ::DispatchMessageA(&next);
                        }
                    }
                }

            #elif defined(__APPLE__)

                //todo macOS clipboard watcher

            #else

                //todo X11 and Wayland clipboard watcher

            #endif

            log(" tty: id: ", std::this_thread::get_id(), " clipboard watcher thread ended");
        }
        void ignite(xipc pipe, si32 mode)
        {
            globals().ipcio = pipe;
            auto& sig_hndl = signal;

            #if defined(_WIN32)

                auto& omode = globals().state[0];
                auto& imode = globals().state[1];

                ok(::GetConsoleMode(STDOUT_FD, &omode), "GetConsoleMode(STDOUT_FD) failed");
                ok(::GetConsoleMode(STDIN_FD , &imode), "GetConsoleMode(STDIN_FD) failed");

                auto inpmode = DWORD{ 0
                              | nt::console::inmode::extended
                              | nt::console::inmode::winsize
                              | nt::console::inmode::mouse
                              };
                ok(::SetConsoleMode(STDIN_FD, inpmode), "SetConsoleMode(STDIN_FD) failed");

                auto outmode = DWORD{ 0
                              | nt::console::outmode::preprocess
                              | nt::console::outmode::vt
                              | nt::console::outmode::no_auto_cr
                              };

                ok(::SetConsoleMode(STDOUT_FD, outmode), "SetConsoleMode(STDOUT_FD) failed");
                ok(::SetConsoleCtrlHandler(reinterpret_cast<PHANDLER_ROUTINE>(sig_hndl), TRUE), "SetConsoleCtrlHandler failed");

            #else

                auto& state = globals().state;
                auto& ipcio = globals().ipcio;
                if (ok(::tcgetattr(STDIN_FD, &state), "tcgetattr(STDIN_FD) failed")) // Set stdin raw mode.
                {
                    auto raw_mode = state;
                    ::cfmakeraw(&raw_mode);
                    ok(::tcsetattr(STDIN_FD, TCSANOW, &raw_mode), "tcsetattr(STDIN_FD, TCSANOW) failed");
                }
                else
                {
                    os::fail("warning: check you are using the proper tty device, try `ssh -tt ...` option");
                }

                ok(::signal(SIGPIPE , SIG_IGN ), "set signal(SIGPIPE ) failed");
                ok(::signal(SIGWINCH, sig_hndl), "set signal(SIGWINCH) failed");
                ok(::signal(SIGTERM , sig_hndl), "set signal(SIGTERM ) failed");
                ok(::signal(SIGHUP  , sig_hndl), "set signal(SIGHUP  ) failed");

            #endif

            os::vgafont_update(mode);
            ::atexit(repair);
            resize();
        }
        auto splice(xipc pipe, si32 mode)
        {
            static auto ocs52head = "\033]52;"sv;
            ignite(pipe, mode);
            auto& ipcio = *pipe;
            auto& alarm = globals().alarm;
            auto  cache = text{};
            auto  brand = text{};
            auto  stamp = datetime::tempus::now();
            auto  vga16 = mode & os::legacy::vga16;
            auto  vtrun = ansi::save_title().altbuf(true).cursor(faux).bpmode(true).setutf(true).set_palette(vga16);
            auto  vtend = ansi::scrn_reset().altbuf(faux).cursor(true).bpmode(faux).load_title().rst_palette(vga16);
            #if not defined(_WIN32) // Use Win32 Console API for mouse tracking on Windows.
            vtrun.vmouse(true);
            vtend.vmouse(faux);
            #endif

            auto proxy = [&](auto& yield) // Redirect clipboard data.
            {
                if (cache.size() || yield.starts_with(ocs52head))
                {
                    auto now = datetime::tempus::now();
                    if (now - stamp > 3s) // Drop outdated cache.
                    {
                        cache.clear();
                        brand.clear();
                    }
                    auto start = cache.size();
                    cache += yield;
                    yield = cache;
                    stamp = now;
                    if (brand.empty())
                    {
                        yield.remove_prefix(ocs52head.size());
                        if (auto p = yield.find(';'); p != view::npos)
                        {
                            brand = yield.substr(0, p++/*;*/);
                            yield.remove_prefix(p);
                            start = ocs52head.size() + p;
                        }
                        else return faux;
                    }
                    else yield.remove_prefix(start);

                    if (auto p = yield.find(ansi::C0_BEL); p != view::npos)
                    {
                        auto temp = view{ cache };
                        auto skip = ocs52head.size() + brand.size() + 1/*;*/;
                        auto data = temp.substr(skip, p + start - skip);
                        if (os::set_clipboard(brand, utf::unbase64(data)))
                        {
                            yield.remove_prefix(p + 1/* C0_BEL */);
                        }
                        else yield = temp; // Forward all out;
                        cache.clear();
                        brand.clear();
                        if (yield.empty()) return faux;
                    }
                    else return faux;
                }
                return true;
            };

            os::send(STDOUT_FD, vtrun);

            auto input = std::thread{ [&]{ reader(mode); } };
            auto clips = std::thread{ [&]{ clipbd(mode); } };
            while (auto yield = ipcio.recv())
            {
                if (proxy(yield) && !os::send(STDOUT_FD, yield)) break;
            }

            ipcio.shut();
            alarm.bell();
            clips.join();
            input.join();

            os::send(STDOUT_FD, cache + vtend);
            std::this_thread::sleep_for(200ms); // Pause to complete consuming/receiving buffered input (e.g. mouse tracking) that has just been canceled.
        }
    };

    template<class Term>
    class vtty // Note: STA.
    {
        Term&                   terminal;
        ipc::stdcon             termlink;
        std::thread             stdinput;
        std::thread             stdwrite;
        std::thread             waitexit;
        testy<twod>             termsize;
        pid_t                   proc_pid;
        fd_t                    prochndl;
        fd_t                    srv_hndl;
        fd_t                    ref_hndl;
        text                    writebuf;
        std::mutex              writemtx;
        std::condition_variable writesyn;

        #if defined(_WIN32)

            #include "consrv.hpp"
            //todo con_serv is a termlink
            consrv con_serv{ terminal, srv_hndl };

        #endif

    public:
        vtty(Term& uiterminal)
            : terminal{ uiterminal },
              prochndl{ INVALID_FD },
              srv_hndl{ INVALID_FD },
              ref_hndl{ INVALID_FD },
              proc_pid{            }
        { }
       ~vtty()
        {
            log("vtty: dtor started");
            stop();
            log("vtty: dtor complete");
        }

        operator bool () { return connected(); }

        bool connected()
        {
            #if defined(_WIN32)
                return srv_hndl != INVALID_FD;
            #else
                return !!termlink;
            #endif
        }
        // vtty: Cleaning in order to be able to restart.
        void cleanup()
        {
            if (stdwrite.joinable())
            {
                writesyn.notify_one();
                log("vtty: id: ", stdwrite.get_id(), " writing thread joining");
                stdwrite.join();
            }
            if (stdinput.joinable())
            {
                log("vtty: id: ", stdinput.get_id(), " reading thread joining");
                stdinput.join();
            }
            if (waitexit.joinable())
            {
                auto id = waitexit.get_id();
                log("vtty: id: ", id, " child process waiter thread joining");
                waitexit.join();
                log("vtty: id: ", id, " child process waiter thread joined");
            }
            auto guard = std::lock_guard{ writemtx };
            termlink = {};
            writebuf = {};
        }
        auto wait_child()
        {
            auto guard = std::lock_guard{ writemtx };
            auto exit_code = si32{};
            log("vtty: wait child process ", proc_pid);

            if (proc_pid != 0)
            {
            #if defined(_WIN32)

                os::close( srv_hndl );
                os::close( ref_hndl );
                con_serv.stop();
                auto code = DWORD{ 0 };
                if (!::GetExitCodeProcess(prochndl, &code))
                {
                    log("vtty: GetExitCodeProcess() return code: ", ::GetLastError());
                }
                else if (code == STILL_ACTIVE)
                {
                    log("vtty: child process still running");
                    auto result = WAIT_OBJECT_0 == ::WaitForSingleObject(prochndl, 10000 /*10 seconds*/);
                    if (!result || !::GetExitCodeProcess(prochndl, &code))
                    {
                        ::TerminateProcess(prochndl, 0);
                        code = 0;
                    }
                }
                else log("vtty: child process exit code 0x", utf::to_hex(code), " (", code, ")");
                exit_code = code;
                os::close(prochndl);

            #else

                termlink.stop();
                auto status = int{};
                ok(::kill(proc_pid, SIGKILL), "kill(pid, SIGKILL) failed");
                ok(::waitpid(proc_pid, &status, 0), "waitpid(pid) failed"); // Wait for the child to avoid zombies.
                if (WIFEXITED(status))
                {
                    exit_code = WEXITSTATUS(status);
                    log("vtty: child process exit code ", exit_code);
                }
                else
                {
                    exit_code = 0;
                    log("vtty: warning: child process exit code not detected");
                }

            #endif
            }
            log("vtty: child waiting complete");
            return exit_code;
        }
        void start(twod winsz)
        {
            auto cwd     = terminal.curdir;
            auto cmdline = terminal.cmdarg;
            utf::change(cmdline, "\\\"", "\"");
            log("vtty: new child process: '", utf::debase(cmdline), "' at the ", cwd.empty() ? "current working directory"s
                                                                                             : "'" + cwd + "'");
            #if defined(_WIN32)

                termsize(winsz);
                auto startinf = STARTUPINFOEX{ sizeof(STARTUPINFOEX) };
                auto procsinf = PROCESS_INFORMATION{};
                auto attrbuff = std::vector<uint8_t>{};
                auto attrsize = SIZE_T{ 0 };

                srv_hndl = nt::console::handle("\\Device\\ConDrv\\Server");
                ref_hndl = nt::console::handle(srv_hndl, "\\Reference");

                if (ERROR_SUCCESS != nt::ioctl(nt::console::op::set_server_information, srv_hndl, con_serv.events.ondata))
                {
                    auto errcode = os::error();
                    log("vtty: console server creation error ", errcode);
                    terminal.onexit(errcode, "Console server creation error");
                    return;
                }

                con_serv.start();
                startinf.StartupInfo.dwX = 0;
                startinf.StartupInfo.dwY = 0;
                startinf.StartupInfo.dwXCountChars = 0;
                startinf.StartupInfo.dwYCountChars = 0;
                startinf.StartupInfo.dwXSize = winsz.x;
                startinf.StartupInfo.dwYSize = winsz.y;
                startinf.StartupInfo.dwFillAttribute = 1;
                startinf.StartupInfo.hStdInput  = nt::console::handle(srv_hndl, "\\Input",  true);
                startinf.StartupInfo.hStdOutput = nt::console::handle(srv_hndl, "\\Output", true);
                startinf.StartupInfo.hStdError  = nt::console::handle(startinf.StartupInfo.hStdOutput);
                startinf.StartupInfo.dwFlags = STARTF_USESTDHANDLES
                                             | STARTF_USESIZE
                                             | STARTF_USEPOSITION
                                             | STARTF_USECOUNTCHARS
                                             | STARTF_USEFILLATTRIBUTE;
                ::InitializeProcThreadAttributeList(nullptr, 2, 0, &attrsize);
                attrbuff.resize(attrsize);
                startinf.lpAttributeList = reinterpret_cast<LPPROC_THREAD_ATTRIBUTE_LIST>(attrbuff.data());
                ::InitializeProcThreadAttributeList(startinf.lpAttributeList, 2, 0, &attrsize);
                ::UpdateProcThreadAttribute(startinf.lpAttributeList,
                                            0,
                                            PROC_THREAD_ATTRIBUTE_HANDLE_LIST,
                                           &startinf.StartupInfo.hStdInput,
                                     sizeof(startinf.StartupInfo.hStdInput) * 3,
                                            nullptr,
                                            nullptr);
                ::UpdateProcThreadAttribute(startinf.lpAttributeList,
                                            0,
                                            ProcThreadAttributeValue(sizeof("Reference"), faux, true, faux),
                                           &ref_hndl,
                                     sizeof(ref_hndl),
                                            nullptr,
                                            nullptr);
                auto ret = ::CreateProcessA(nullptr,                             // lpApplicationName
                                            cmdline.data(),                      // lpCommandLine
                                            nullptr,                             // lpProcessAttributes
                                            nullptr,                             // lpThreadAttributes
                                            TRUE,                                // bInheritHandles
                                            EXTENDED_STARTUPINFO_PRESENT,        // dwCreationFlags (override startupInfo type)
                                            nullptr,                             // lpEnvironment
                                            cwd.empty() ? nullptr
                                                        : (LPCSTR)(cwd.c_str()), // lpCurrentDirectory
                                           &startinf.StartupInfo,                // lpStartupInfo (ptr to STARTUPINFOEX)
                                           &procsinf);                           // lpProcessInformation
                if (ret == 0)
                {
                    auto errcode = os::error();
                    log("vtty: child process creation error ", errcode);
                    os::close( srv_hndl );
                    os::close( ref_hndl );
                    con_serv.stop();
                    terminal.onexit(errcode, "Process creation error \n"s
                                             " cwd: "s + (cwd.empty() ? "not specified"s : cwd) + " \n"s
                                             " cmd: "s + cmdline + " "s);
                    return;
                }

                os::close( procsinf.hThread );
                prochndl = procsinf.hProcess;
                proc_pid = procsinf.dwProcessId;
                waitexit = std::thread([&]
                {
                    os::select(prochndl, []{ log("vtty: child process terminated"); });
                    if (srv_hndl != INVALID_FD)
                    {
                        auto exit_code = wait_child();
                        terminal.onexit(exit_code);
                    }
                    log("vtty: child process waiter ended");
                });

            #else

                auto fdm = ::posix_openpt(O_RDWR | O_NOCTTY); // Get master PTY.
                auto rc1 = ::grantpt     (fdm);               // Grant master PTY file access.
                auto rc2 = ::unlockpt    (fdm);               // Unlock master PTY.
                auto fds = ::open(ptsname(fdm), O_RDWR);      // Open slave PTY via string ptsname(fdm).

                termlink = { fdm, fdm };
                termsize = {};
                resize(winsz);

                proc_pid = ::fork();
                if (proc_pid == 0) // Child branch.
                {
                    os::close(fdm); // os::fdcleanup();
                    auto rc0 = ::setsid(); // Make the current process a new session leader, return process group id.

                    // In order to receive WINCH signal make fds the controlling
                    // terminal of the current process.
                    // Current process must be a session leader (::setsid()) and not have
                    // a controlling terminal already.
                    // arg = 0: 1 - to stole fds from another process, it doesn't matter here.
                    auto rc1 = ::ioctl(fds, TIOCSCTTY, 0);

                    ::signal(SIGINT,  SIG_DFL); // Reset control signals to default values.
                    ::signal(SIGQUIT, SIG_DFL); //
                    ::signal(SIGTSTP, SIG_DFL); //
                    ::signal(SIGTTIN, SIG_DFL); //
                    ::signal(SIGTTOU, SIG_DFL); //
                    ::signal(SIGCHLD, SIG_DFL); //

                    if (cwd.size())
                    {
                        auto err = std::error_code{};
                        fs::current_path(cwd, err);
                        if (err) std::cerr << "vtty: failed to change current working directory to '" << cwd << "', error code: " << err.value() << "\n" << std::flush;
                        else     std::cerr << "vtty: change current working directory to '" << cwd << "'" << "\n" << std::flush;
                    }

                    ::dup2(fds, STDIN_FD ); // Assign stdio lines atomically
                    ::dup2(fds, STDOUT_FD); // = close(new);
                    ::dup2(fds, STDERR_FD); // fcntl(old, F_DUPFD, new)
                    os::fdcleanup();

                    ::setenv("TERM", "xterm-256color", 1); //todo too hacky
                    if (os::env::get("TERM_PROGRAM") == "Apple_Terminal")
                    {
                        ::setenv("TERM_PROGRAM", "vtm", 1);
                    }

                    os::execvp(cmdline);
                    auto errcode = errno;
                    std::cerr << ansi::bgc(reddk).fgc(whitelt).add("Process creation error ").add(errcode).add(" \n"s
                                                                   " cwd: "s + (cwd.empty() ? "not specified"s : cwd) + " \n"s
                                                                   " cmd: "s + cmdline + " "s).nil() << std::flush;
                    os::exit(errcode);
                }

                // Parent branch.
                os::close(fds);

                stdinput = std::thread([&] { read_socket_thread(); });

            #endif

            stdwrite = std::thread([&] { send_socket_thread(); });
            writesyn.notify_one(); // Flush temp buffer.

            log("vtty: new vtty created with size ", winsz);
        }
        void stop()
        {
            if (connected())
            {
                wait_child();
            }
            cleanup();
        }
        void read_socket_thread()
        {
            #if not defined(_WIN32)

                log("vtty: id: ", stdinput.get_id(), " reading thread started");
                auto flow = text{};
                while (termlink)
                {
                    auto shot = termlink.recv();
                    if (shot && termlink)
                    {
                        flow += shot;
                        auto crop = ansi::purify(flow);
                        terminal.ondata(crop);
                        flow.erase(0, crop.size()); // Delete processed data.
                    }
                    else break;
                }
                if (termlink)
                {
                    auto exit_code = wait_child();
                    terminal.onexit(exit_code);
                }
                log("vtty: id: ", stdinput.get_id(), " reading thread ended");

            #endif
        }
        void send_socket_thread()
        {
            log("vtty: id: ", stdwrite.get_id(), " writing thread started");
            auto guard = std::unique_lock{ writemtx };
            auto cache = text{};
            while ((void)writesyn.wait(guard, [&]{ return writebuf.size() || !connected(); }), connected())
            {
                std::swap(cache, writebuf);
                guard.unlock();
                #if defined(_WIN32)

                    con_serv.events.write(cache);
                    cache.clear();

                #else

                    if (termlink.send(cache)) cache.clear();
                    else                      break;

                #endif
                guard.lock();
            }
            log("vtty: id: ", stdwrite.get_id(), " writing thread ended");
        }
        void resize(twod const& newsize)
        {
            if (connected() && termsize(newsize))
            {
                #if defined(_WIN32)

                    con_serv.resize(termsize);

                #else

                    auto winsz = winsize{};
                    winsz.ws_col = newsize.x;
                    winsz.ws_row = newsize.y;
                    ok(::ioctl(termlink.handle.w, TIOCSWINSZ, &winsz), "ioctl(termlink.get(), TIOCSWINSZ) failed");

                #endif
            }
        }
        void reset()
        {
            #if defined(_WIN32)
                con_serv.reset();
            #endif
        }
        void keybd(input::hids& gear, bool decckm)
        {
            #if defined(_WIN32)
                con_serv.events.keybd(gear, decckm);
            #endif
        }
        void mouse(input::hids& gear, bool moved, twod const& coord)
        {
            #if defined(_WIN32)
                con_serv.events.mouse(gear, moved, coord);
            #endif
        }
        void write(view data)
        {
            auto guard = std::lock_guard{ writemtx };
            writebuf += data;
            if (connected()) writesyn.notify_one();
        }
        void undo(bool undoredo)
        {
            #if defined(_WIN32)
                con_serv.events.undo(undoredo);
            #endif
        }
    };

    namespace direct
    {
        class vtty
        {
            fd_t                      prochndl{ INVALID_FD };
            pid_t                     proc_pid{};
            ipc::stdcon               termlink{};
            std::thread               stdinput{};
            std::thread               stdwrite{};
            std::function<void(view)> receiver{};
            std::function<void(si32)> shutdown{};
            std::function<void(si32)> preclose{};
            text                      writebuf{};
            std::mutex                writemtx{};
            std::condition_variable   writesyn{};

        public:
           ~vtty()
            {
                log("dtvt: dtor started");
                if (termlink)
                {
                    stop();
                }
                if (stdwrite.joinable())
                {
                    writesyn.notify_one();
                    log("dtvt: id: ", stdwrite.get_id(), " writing thread joining");
                    stdwrite.join();
                }
                if (stdinput.joinable())
                {
                    log("dtvt: id: ", stdinput.get_id(), " reading thread joining");
                    stdinput.join();
                }
                log("dtvt: dtor complete");
            }

            operator bool () { return termlink; }

            auto start(text cwd, text cmdline, text config, std::function<void(view)> input_hndl,
                                                            std::function<void(si32)> preclose_hndl,
                                                            std::function<void(si32)> shutdown_hndl)
            {
                receiver = input_hndl;
                preclose = preclose_hndl;
                shutdown = shutdown_hndl;
                utf::change(cmdline, "\\\"", "'");
                log("dtvt: new child process: '", utf::debase(cmdline), "' at the ", cwd.empty() ? "current working directory"s
                                                                                                 : "'" + cwd + "'");
                #if defined(_WIN32)

                    auto s_pipe_r = INVALID_FD;
                    auto s_pipe_w = INVALID_FD;
                    auto m_pipe_r = INVALID_FD;
                    auto m_pipe_w = INVALID_FD;
                    auto startinf = STARTUPINFOEX{ sizeof(STARTUPINFOEX) };
                    auto procsinf = PROCESS_INFORMATION{};
                    auto attrbuff = std::vector<uint8_t>{};
                    auto attrsize = SIZE_T{ 0 };
                    auto stdhndls = std::array<HANDLE, 2>{};

                    auto tunnel = [&]
                    {
                        auto sa = SECURITY_ATTRIBUTES{};
                        sa.nLength = sizeof(SECURITY_ATTRIBUTES);
                        sa.lpSecurityDescriptor = NULL;
                        sa.bInheritHandle = TRUE;
                        if (::CreatePipe(&s_pipe_r, &m_pipe_w, &sa, 0)
                         && ::CreatePipe(&m_pipe_r, &s_pipe_w, &sa, 0))
                        {
                            os::legacy::send_dmd(m_pipe_w, config);

                            startinf.StartupInfo.dwFlags    = STARTF_USESTDHANDLES;
                            startinf.StartupInfo.hStdInput  = s_pipe_r;
                            startinf.StartupInfo.hStdOutput = s_pipe_w;
                            return true;
                        }
                        else
                        {
                            os::close(m_pipe_w);
                            os::close(m_pipe_r);
                            return faux;
                        }
                    };
                    auto fillup = [&]
                    {
                        stdhndls[0] = s_pipe_r;
                        stdhndls[1] = s_pipe_w;
                        ::InitializeProcThreadAttributeList(nullptr, 1, 0, &attrsize);
                        attrbuff.resize(attrsize);
                        startinf.lpAttributeList = reinterpret_cast<LPPROC_THREAD_ATTRIBUTE_LIST>(attrbuff.data());

                        if (::InitializeProcThreadAttributeList(startinf.lpAttributeList, 1, 0, &attrsize)
                         && ::UpdateProcThreadAttribute( startinf.lpAttributeList,
                                                         0,
                                                         PROC_THREAD_ATTRIBUTE_HANDLE_LIST,
                                                         &stdhndls,
                                                         sizeof(stdhndls),
                                                         nullptr,
                                                         nullptr))
                        {
                            return true;
                        }
                        else return faux;
                    };
                    auto create = [&]
                    {
                        return ::CreateProcessA( nullptr,                             // lpApplicationName
                                                 cmdline.data(),                      // lpCommandLine
                                                 nullptr,                             // lpProcessAttributes
                                                 nullptr,                             // lpThreadAttributes
                                                 TRUE,                                // bInheritHandles
                                                 DETACHED_PROCESS |                   // create without attached console, dwCreationFlags
                                                 EXTENDED_STARTUPINFO_PRESENT,        // override startupInfo type
                                                 nullptr,                             // lpEnvironment
                                                 cwd.empty() ? nullptr
                                                             : (LPCSTR)(cwd.c_str()), // lpCurrentDirectory
                                                 &startinf.StartupInfo,               // lpStartupInfo (ptr to STARTUPINFO)
                                                 &procsinf);                          // lpProcessInformation
                    };

                    if (tunnel()
                     && fillup()
                     && create())
                    {
                        os::close( procsinf.hThread );
                        prochndl = procsinf.hProcess;
                        proc_pid = procsinf.dwProcessId;
                        termlink = { m_pipe_r, m_pipe_w };
                    }
                    else log("dtvt: child process creation error ", ::GetLastError());

                    os::close(s_pipe_w); // Close inheritable handles to avoid deadlocking at process exit.
                    os::close(s_pipe_r); // Only when all write handles to the pipe are closed, the ReadFile function returns zero.

                #else

                    fd_t to_server[2] = { INVALID_FD, INVALID_FD };
                    fd_t to_client[2] = { INVALID_FD, INVALID_FD };
                    ok(::pipe(to_server), "dtvt: server ipc unexpected result");
                    ok(::pipe(to_client), "dtvt: client ipc unexpected result");

                    termlink = { to_server[0], to_client[1] };
                    os::legacy::send_dmd(to_client[1], config);

                    proc_pid = ::fork();
                    if (proc_pid == 0) // Child branch.
                    {
                        os::close(to_client[1]); // os::fdcleanup();
                        os::close(to_server[0]);

                        ::signal(SIGINT,  SIG_DFL); // Reset control signals to default values.
                        ::signal(SIGQUIT, SIG_DFL); //
                        ::signal(SIGTSTP, SIG_DFL); //
                        ::signal(SIGTTIN, SIG_DFL); //
                        ::signal(SIGTTOU, SIG_DFL); //
                        ::signal(SIGCHLD, SIG_DFL); //

                        ::dup2(to_client[0], STDIN_FD ); // Assign stdio lines atomically
                        ::dup2(to_server[1], STDOUT_FD); // = close(new); fcntl(old, F_DUPFD, new).
                        os::fdcleanup();

                        if (cwd.size())
                        {
                            auto err = std::error_code{};
                            fs::current_path(cwd, err);
                            //todo use dtvt to log
                            //if (err) os::fail("dtvt: failed to change current working directory to '", cwd, "', error code: ", err.value());
                            //else     log("dtvt: change current working directory to '", cwd, "'");
                        }

                        os::execvp(cmdline);
                        auto errcode = errno;
                        //todo use dtvt to log
                        //os::fail("dtvt: exec error");
                        ::close(STDOUT_FD);
                        ::close(STDIN_FD );
                        os::exit(errcode);
                    }

                    // Parent branch.
                    os::close(to_client[0]);
                    os::close(to_server[1]);

                #endif

                if (config.size())
                {
                    auto guard = std::lock_guard{ writemtx };
                    writebuf = config + writebuf;
                }

                stdinput = std::thread([&] { read_socket_thread(); });
                stdwrite = std::thread([&] { send_socket_thread(); });

                if (termlink) log("dtvt: vtty created: proc_pid ", proc_pid);
                writesyn.notify_one(); // Flush temp buffer.

                return proc_pid;
            }
            void stop()
            {
                termlink.shut();
                writesyn.notify_one();
            }
            auto wait_child()
            {
                //auto guard = std::lock_guard{ writemtx };
                auto exit_code = si32{};
                log("dtvt: wait child process, tty=", termlink);
                if (termlink)
                {
                    termlink.shut();
                }

                if (proc_pid != 0)
                {
                    #if defined(_WIN32)

                        auto code = DWORD{ 0 };
                        if (!::GetExitCodeProcess(prochndl, &code))
                        {
                            log("dtvt: GetExitCodeProcess() return code: ", ::GetLastError());
                        }
                        else if (code == STILL_ACTIVE)
                        {
                            log("dtvt: child process still running");
                            auto result = WAIT_OBJECT_0 == ::WaitForSingleObject(prochndl, 10000 /*10 seconds*/);
                            if (!result || !::GetExitCodeProcess(prochndl, &code))
                            {
                                ::TerminateProcess(prochndl, 0);
                                code = 0;
                            }
                        }
                        else log("dtvt: child process exit code ", code);
                        exit_code = code;
                        os::close(prochndl);

                    #else

                        int status;
                        ok(::kill(proc_pid, SIGKILL), "kill(pid, SIGKILL) failed");
                        ok(::waitpid(proc_pid, &status, 0), "waitpid(pid) failed"); // Wait for the child to avoid zombies.
                        if (WIFEXITED(status))
                        {
                            exit_code = WEXITSTATUS(status);
                            log("dtvt: child process exit code ", exit_code);
                        }
                        else
                        {
                            exit_code = 0;
                            log("dtvt: warning: child process exit code not detected");
                        }

                    #endif
                }
                log("dtvt: child waiting complete");
                return exit_code;
            }
            void read_socket_thread()
            {
                log("dtvt: id: ", stdinput.get_id(), " reading thread started");

                ansi::dtvt::binary::stream::reading_loop(termlink, receiver);

                preclose(0); //todo send msg from the client app
                shutdown(wait_child());
                log("dtvt: id: ", stdinput.get_id(), " reading thread ended");
            }
            void send_socket_thread()
            {
                log("dtvt: id: ", stdwrite.get_id(), " writing thread started");
                auto guard = std::unique_lock{ writemtx };
                auto cache = text{};
                while ((void)writesyn.wait(guard, [&]{ return writebuf.size() || !termlink; }), termlink)
                {
                    std::swap(cache, writebuf);
                    guard.unlock();
                    if (termlink.send(cache)) cache.clear();
                    else                      break;
                    guard.lock();
                }
                //todo block dtvt-logs after termlink stops
                //guard.unlock(); // To avoid debug output deadlocking. See ui::dtvt::request_debug() - e2::debug::logs
                log("dtvt: id: ", stdwrite.get_id(), " writing thread ended");
            }
            void output(view data)
            {
                auto guard = std::lock_guard{ writemtx };
                writebuf += data;
                if (termlink) writesyn.notify_one();
            }
        };
    }
}