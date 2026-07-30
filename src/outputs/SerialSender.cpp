#include "SerialSender.h"

#include "spdlog/spdlog.h"

#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#else
#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#endif

#ifndef _WIN32
namespace
{
	// Linux defines B250000 as a non-POSIX extension specifically for
	// DMX/MIDI use (alongside the standard rates); anything unrecognised
	// falls back to a safe default rather than failing to open at all.
	speed_t ToSpeed(uint32_t baud)
	{
		switch (baud)
		{
			case 9600: return B9600;
			case 19200: return B19200;
			case 38400: return B38400;
			case 57600: return B57600;
			case 115200: return B115200;
			case 230400: return B230400;
#ifdef B250000
			case 250000: return B250000;
#endif
			default: return B57600;
		}
	}
}
#endif

bool SerialSender::Open(std::string const& portName, uint32_t baudRate, bool twoStopBits)
{
	Close();
	auto logger = spdlog::get("miniplayer");

#ifdef _WIN32
	// COM10 and higher need the \\.\ prefix to open at all; harmless for COM1-9.
	std::string const path = "\\\\.\\" + portName;

	HANDLE handle = CreateFileA(path.c_str(), GENERIC_WRITE, 0, nullptr,
		OPEN_EXISTING, 0, nullptr);
	if (handle == INVALID_HANDLE_VALUE)
	{
		if (logger) logger->error("Unable to open serial port {}: error {}", portName, GetLastError());
		return false;
	}

	DCB dcb{};
	dcb.DCBlength = sizeof(dcb);
	if (!GetCommState(handle, &dcb))
	{
		if (logger) logger->error("Unable to read serial state for {}", portName);
		CloseHandle(handle);
		return false;
	}
	dcb.BaudRate = baudRate;
	dcb.ByteSize = 8;
	dcb.Parity = NOPARITY;
	dcb.StopBits = twoStopBits ? TWOSTOPBITS : ONESTOPBIT;
	dcb.fBinary = TRUE;
	dcb.fParity = FALSE;
	dcb.fOutxCtsFlow = FALSE;
	dcb.fOutxDsrFlow = FALSE;
	dcb.fDtrControl = DTR_CONTROL_DISABLE;
	dcb.fRtsControl = RTS_CONTROL_DISABLE;
	if (!SetCommState(handle, &dcb))
	{
		if (logger) logger->error("Unable to configure serial port {} at {} baud", portName, baudRate);
		CloseHandle(handle);
		return false;
	}

	// Blocking writes with a generous timeout: frames are small (a few
	// hundred bytes) and sent at most a few dozen times a second, so there is
	// no need for overlapped I/O here.
	COMMTIMEOUTS timeouts{};
	timeouts.WriteTotalTimeoutConstant = 1000;
	SetCommTimeouts(handle, &timeouts);

	m_handle = handle;
	return true;
#else
	int const fd = open(portName.c_str(), O_WRONLY | O_NOCTTY);
	if (fd < 0)
	{
		if (logger) logger->error("Unable to open serial port {}: {}", portName, strerror(errno));
		return false;
	}

	termios tty{};
	if (tcgetattr(fd, &tty) != 0)
	{
		if (logger) logger->error("Unable to read serial state for {}", portName);
		close(fd);
		return false;
	}

	cfmakeraw(&tty);
	speed_t const speed = ToSpeed(baudRate);
	cfsetispeed(&tty, speed);
	cfsetospeed(&tty, speed);

	tty.c_cflag |= (CLOCAL | CREAD);
	tty.c_cflag &= ~PARENB;
	tty.c_cflag &= ~CSIZE;
	tty.c_cflag |= CS8;
	if (twoStopBits)
	{
		tty.c_cflag |= CSTOPB;
	}
	else
	{
		tty.c_cflag &= ~CSTOPB;
	}

	if (tcsetattr(fd, TCSANOW, &tty) != 0)
	{
		if (logger) logger->error("Unable to configure serial port {} at {} baud", portName, baudRate);
		close(fd);
		return false;
	}

	m_fd = fd;
	return true;
#endif
}

void SerialSender::Close()
{
#ifdef _WIN32
	if (m_handle != nullptr)
	{
		CloseHandle(static_cast<HANDLE>(m_handle));
		m_handle = nullptr;
	}
#else
	if (m_fd >= 0)
	{
		close(m_fd);
		m_fd = -1;
	}
#endif
}

bool SerialSender::IsOpen() const
{
#ifdef _WIN32
	return m_handle != nullptr;
#else
	return m_fd >= 0;
#endif
}

bool SerialSender::Send(uint8_t const* data, std::size_t len)
{
#ifdef _WIN32
	if (m_handle == nullptr)
	{
		return false;
	}
	DWORD written{ 0 };
	return WriteFile(static_cast<HANDLE>(m_handle), data, static_cast<DWORD>(len), &written, nullptr)
		&& written == len;
#else
	if (m_fd < 0)
	{
		return false;
	}
	ssize_t const written = write(m_fd, data, len);
	return written == static_cast<ssize_t>(len);
#endif
}
