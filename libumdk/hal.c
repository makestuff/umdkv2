/* 
 * Copyright (C) 2011 Chris McClelland
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *  
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <stdio.h>
#include <usbwrap.h>
#include <libsync.h>
#include <types.h>
#include "error.h"
#include "status.h"

static UsbDeviceHandle *m_deviceHandle = NULL;

UMDKStatus umdkOpen(uint16 vid, uint16 pid) {
	int returnCode;

	if ( m_deviceHandle ) {
		snprintf(
			m_umdkErrorMessage, UMDK_ERR_MAXLENGTH,
			"umdkOpen(): already open!");
		return UMDK_ALREADYOPEN;
	}
	usbInitialise();
	returnCode = usbOpenDevice(vid, pid, 1, 0, 0, &m_deviceHandle);
	if ( returnCode ) {
		snprintf(
			m_umdkErrorMessage, UMDK_ERR_MAXLENGTH,
			"umdkOpen(): usbOpenDevice() failed returnCode %d: %s",
			returnCode, usbStrError());
		return UMDK_USBERR;
	}
	usb_clear_halt(m_deviceHandle, USB_ENDPOINT_OUT | 6);
	usb_clear_halt(m_deviceHandle, USB_ENDPOINT_IN | 8);
	if ( syncBulkEndpoints(m_deviceHandle, SYNC_68) ) {
		snprintf(
			m_umdkErrorMessage, UMDK_ERR_MAXLENGTH,
			"umdkOpen(): Failed to sync bulk endpoints: %s\n",
			syncStrError());
		return UMDK_SYNCERR;
	}
	return UMDK_SUCCESS;
}

void umdkClose(void) {
	if ( m_deviceHandle ) {
		usb_release_interface(m_deviceHandle, 0);
		usb_close(m_deviceHandle);
		m_deviceHandle = NULL;
	}
}

UMDKStatus umdkWrite(const uint8 *bytes, uint32 count, uint32 timeout) {
	int returnCode = usb_bulk_write(
		m_deviceHandle, USB_ENDPOINT_OUT | 6, (char*)bytes, count, timeout);
	if ( returnCode < 0 ) {
		snprintf(
			m_umdkErrorMessage, UMDK_ERR_MAXLENGTH,
			"umdkWrite(): usb_bulk_write() failed returnCode %d: %s",
			returnCode, usb_strerror());
		return UMDK_USBERR;
	} else if ( (uint32)returnCode != count ) {
		snprintf(
			m_umdkErrorMessage, UMDK_ERR_MAXLENGTH,
			"umdkWrite(): usb_bulk_write() wrote %d bytes instead of the expected %lu",
			returnCode, count);
		return UMDK_USBERR;
	}
	return UMDK_SUCCESS;
}

UMDKStatus umdkRead(uint8 *buffer, uint32 count, uint32 timeout) {
	int returnCode = usb_bulk_read(
		m_deviceHandle, USB_ENDPOINT_IN | 8, (char*)buffer, count, timeout);
	if ( returnCode < 0 ) {
		snprintf(
			m_umdkErrorMessage, UMDK_ERR_MAXLENGTH,
			"umdkRead(): usb_bulk_read() failed returnCode %d: %s",
			returnCode, usb_strerror());
		return UMDK_USBERR;
	} else if ( (uint32)returnCode != count ) {
		snprintf(
			m_umdkErrorMessage, UMDK_ERR_MAXLENGTH,
			"umdkRead(): usb_bulk_read() read %d bytes instead of the expected %lu",
			returnCode, count);
		return UMDK_USBERR;
	}
	return UMDK_SUCCESS;
}
