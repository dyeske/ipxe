/*
 * Copyright (C) 2014 Michael Brown <mbrown@fensystems.co.uk>.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 *
 * You can also choose to distribute this program under the terms of
 * the Unmodified Binary Distribution Licence (as given in the file
 * COPYING.UBDL), provided that you have satisfied its requirements.
 */

FILE_LICENCE ( GPL2_OR_LATER_OR_UBDL );

#include <string.h>
#include <strings.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <ipxe/netdevice.h>
#include <ipxe/ethernet.h>
#include <ipxe/if_ether.h>
#include <ipxe/umalloc.h>
#include <ipxe/efi/efi.h>
#include <ipxe/efi/efi_driver.h>
#include <ipxe/efi/efi_pci.h>
#include <ipxe/efi/efi_utils.h>
#include <ipxe/efi/Protocol/NetworkInterfaceIdentifier.h>
#include <ipxe/efi/IndustryStandard/Acpi10.h>
#include "nii.h"

/** @file
 *
 * NII driver
 *
 */

/* Error numbers generated by NII */
#define EIO_INVALID_CDB __einfo_error ( EINFO_EIO_INVALID_CDB )
#define EINFO_EIO_INVALID_CDB						      \
	__einfo_uniqify ( EINFO_EIO, PXE_STATCODE_INVALID_CDB,		      \
			  "Invalid CDB" )
#define EIO_INVALID_CPB __einfo_error ( EINFO_EIO_INVALID_CPB )
#define EINFO_EIO_INVALID_CPB						      \
	__einfo_uniqify ( EINFO_EIO, PXE_STATCODE_INVALID_CPB,		      \
			  "Invalid CPB" )
#define EIO_BUSY __einfo_error ( EINFO_EIO_BUSY )
#define EINFO_EIO_BUSY							      \
	__einfo_uniqify ( EINFO_EIO, PXE_STATCODE_BUSY,			      \
			  "Busy" )
#define EIO_QUEUE_FULL __einfo_error ( EINFO_EIO_QUEUE_FULL )
#define EINFO_EIO_QUEUE_FULL						      \
	__einfo_uniqify ( EINFO_EIO, PXE_STATCODE_QUEUE_FULL,		      \
			  "Queue full" )
#define EIO_ALREADY_STARTED __einfo_error ( EINFO_EIO_ALREADY_STARTED )
#define EINFO_EIO_ALREADY_STARTED					      \
	__einfo_uniqify ( EINFO_EIO, PXE_STATCODE_ALREADY_STARTED,	      \
			  "Already started" )
#define EIO_NOT_STARTED __einfo_error ( EINFO_EIO_NOT_STARTED )
#define EINFO_EIO_NOT_STARTED						      \
	__einfo_uniqify ( EINFO_EIO, PXE_STATCODE_NOT_STARTED,		      \
			  "Not started" )
#define EIO_NOT_SHUTDOWN __einfo_error ( EINFO_EIO_NOT_SHUTDOWN )
#define EINFO_EIO_NOT_SHUTDOWN						      \
	__einfo_uniqify ( EINFO_EIO, PXE_STATCODE_NOT_SHUTDOWN,		      \
			  "Not shutdown" )
#define EIO_ALREADY_INITIALIZED __einfo_error ( EINFO_EIO_ALREADY_INITIALIZED )
#define EINFO_EIO_ALREADY_INITIALIZED					      \
	__einfo_uniqify ( EINFO_EIO, PXE_STATCODE_ALREADY_INITIALIZED,	      \
			  "Already initialized" )
#define EIO_NOT_INITIALIZED __einfo_error ( EINFO_EIO_NOT_INITIALIZED )
#define EINFO_EIO_NOT_INITIALIZED					      \
	__einfo_uniqify ( EINFO_EIO, PXE_STATCODE_NOT_INITIALIZED,	      \
			  "Not initialized" )
#define EIO_DEVICE_FAILURE __einfo_error ( EINFO_EIO_DEVICE_FAILURE )
#define EINFO_EIO_DEVICE_FAILURE					      \
	__einfo_uniqify ( EINFO_EIO, PXE_STATCODE_DEVICE_FAILURE,	      \
			  "Device failure" )
#define EIO_NVDATA_FAILURE __einfo_error ( EINFO_EIO_NVDATA_FAILURE )
#define EINFO_EIO_NVDATA_FAILURE					      \
	__einfo_uniqify ( EINFO_EIO, PXE_STATCODE_NVDATA_FAILURE,	      \
			  "Non-volatile data failure" )
#define EIO_UNSUPPORTED __einfo_error ( EINFO_EIO_UNSUPPORTED )
#define EINFO_EIO_UNSUPPORTED						      \
	__einfo_uniqify ( EINFO_EIO, PXE_STATCODE_UNSUPPORTED,		      \
			  "Unsupported" )
#define EIO_BUFFER_FULL __einfo_error ( EINFO_EIO_BUFFER_FULL )
#define EINFO_EIO_BUFFER_FULL						      \
	__einfo_uniqify ( EINFO_EIO, PXE_STATCODE_BUFFER_FULL,		      \
			  "Buffer full" )
#define EIO_INVALID_PARAMETER __einfo_error ( EINFO_EIO_INVALID_PARAMETER )
#define EINFO_EIO_INVALID_PARAMETER					      \
	__einfo_uniqify ( EINFO_EIO, PXE_STATCODE_INVALID_PARAMETER,	      \
			  "Invalid parameter" )
#define EIO_INVALID_UNDI __einfo_error ( EINFO_EIO_INVALID_UNDI )
#define EINFO_EIO_INVALID_UNDI						      \
	__einfo_uniqify ( EINFO_EIO, PXE_STATCODE_INVALID_UNDI,		      \
			  "Invalid UNDI" )
#define EIO_IPV4_NOT_SUPPORTED __einfo_error ( EINFO_EIO_IPV4_NOT_SUPPORTED )
#define EINFO_EIO_IPV4_NOT_SUPPORTED					      \
	__einfo_uniqify ( EINFO_EIO, PXE_STATCODE_IPV4_NOT_SUPPORTED,	      \
			  "IPv4 not supported" )
#define EIO_IPV6_NOT_SUPPORTED __einfo_error ( EINFO_EIO_IPV6_NOT_SUPPORTED )
#define EINFO_EIO_IPV6_NOT_SUPPORTED					      \
	__einfo_uniqify ( EINFO_EIO, PXE_STATCODE_IPV6_NOT_SUPPORTED,	      \
			  "IPv6 not supported" )
#define EIO_NOT_ENOUGH_MEMORY __einfo_error ( EINFO_EIO_NOT_ENOUGH_MEMORY )
#define EINFO_EIO_NOT_ENOUGH_MEMORY					      \
	__einfo_uniqify ( EINFO_EIO, PXE_STATCODE_NOT_ENOUGH_MEMORY,	      \
			  "Not enough memory" )
#define EIO_NO_DATA __einfo_error ( EINFO_EIO_NO_DATA )
#define EINFO_EIO_NO_DATA						      \
	__einfo_uniqify ( EINFO_EIO, PXE_STATCODE_NO_DATA,		      \
			  "No data" )
#define EIO_STAT( stat )						      \
	EUNIQ ( EINFO_EIO, -(stat), EIO_INVALID_CDB, EIO_INVALID_CPB,	      \
		EIO_BUSY, EIO_QUEUE_FULL, EIO_ALREADY_STARTED,		      \
		EIO_NOT_STARTED, EIO_NOT_SHUTDOWN, EIO_ALREADY_INITIALIZED,   \
		EIO_NOT_INITIALIZED, EIO_DEVICE_FAILURE, EIO_NVDATA_FAILURE,  \
		EIO_UNSUPPORTED, EIO_BUFFER_FULL, EIO_INVALID_PARAMETER,      \
		EIO_INVALID_UNDI, EIO_IPV4_NOT_SUPPORTED,		      \
		EIO_IPV6_NOT_SUPPORTED, EIO_NOT_ENOUGH_MEMORY, EIO_NO_DATA )

/** Maximum PCI BAR
 *
 * This is defined in <ipxe/efi/IndustryStandard/Pci22.h>, but we
 * can't #include that since it collides with <ipxe/pci.h>.
 */
#define PCI_MAX_BAR 6

/** An NII memory mapping */
struct nii_mapping {
	/** List of mappings */
	struct list_head list;
	/** Mapped address */
	UINT64 addr;
	/** Mapping cookie created by PCI I/O protocol */
	VOID *mapping;
};

/** An NII NIC */
struct nii_nic {
	/** EFI device */
	struct efi_device *efidev;
	/** Network interface identifier protocol */
	EFI_NETWORK_INTERFACE_IDENTIFIER_PROTOCOL *nii;
	/** !PXE structure */
	PXE_SW_UNDI *undi;
	/** Entry point */
	EFIAPI VOID ( * issue ) ( UINT64 cdb );
	/** Generic device */
	struct device dev;

	/** PCI device */
	EFI_HANDLE pci_device;
	/** PCI I/O protocol */
	EFI_PCI_IO_PROTOCOL *pci_io;
	/** Memory BAR */
	unsigned int mem_bar;
	/** I/O BAR */
	unsigned int io_bar;

	/** Broadcast address */
	PXE_MAC_ADDR broadcast;
	/** Maximum packet length */
	size_t mtu;

	/** Hardware transmit/receive buffer */
	userptr_t buffer;
	/** Hardware transmit/receive buffer length */
	size_t buffer_len;

	/** Saved task priority level */
	EFI_TPL saved_tpl;

	/** Media status is supported */
	int media;

	/** Current transmit buffer */
	struct io_buffer *txbuf;
	/** Current receive buffer */
	struct io_buffer *rxbuf;

	/** Mapping list */
	struct list_head mappings;
};

/** Maximum number of received packets per poll */
#define NII_RX_QUOTA 4

/**
 * Open PCI I/O protocol and identify BARs
 *
 * @v nii		NII NIC
 * @ret rc		Return status code
 */
static int nii_pci_open ( struct nii_nic *nii ) {
	EFI_BOOT_SERVICES *bs = efi_systab->BootServices;
	EFI_HANDLE device = nii->efidev->device;
	EFI_HANDLE pci_device;
	union {
		EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR *acpi;
		void *resource;
	} desc;
	int bar;
	EFI_STATUS efirc;
	int rc;

	/* Locate PCI I/O protocol */
	if ( ( rc = efi_locate_device ( device, &efi_pci_io_protocol_guid,
					&pci_device, 0 ) ) != 0 ) {
		DBGC ( nii, "NII %s could not locate PCI I/O protocol: %s\n",
		       nii->dev.name, strerror ( rc ) );
		goto err_locate;
	}
	nii->pci_device = pci_device;

	/* Open PCI I/O protocol
	 *
	 * We cannot open this safely as a by-driver open, since doing
	 * so would disconnect the underlying NII driver.  We must
	 * therefore use an unsafe open.
	 */
	if ( ( rc = efi_open_unsafe ( pci_device, &efi_pci_io_protocol_guid,
				      &nii->pci_io ) ) != 0 ) {
		DBGC ( nii, "NII %s could not open PCI I/O protocol: %s\n",
		       nii->dev.name, strerror ( rc ) );
		goto err_open;
	}

	/* Identify memory and I/O BARs */
	nii->mem_bar = PCI_MAX_BAR;
	nii->io_bar = PCI_MAX_BAR;
	for ( bar = ( PCI_MAX_BAR - 1 ) ; bar >= 0 ; bar-- ) {
		efirc = nii->pci_io->GetBarAttributes ( nii->pci_io, bar, NULL,
							&desc.resource );
		if ( efirc == EFI_UNSUPPORTED ) {
			/* BAR not present; ignore */
			continue;
		}
		if ( efirc != 0 ) {
			rc = -EEFI ( efirc );
			DBGC ( nii, "NII %s could not get BAR %d attributes: "
			       "%s\n", nii->dev.name, bar, strerror ( rc ) );
			goto err_get_bar_attributes;
		}
		if ( desc.acpi->ResType == ACPI_ADDRESS_SPACE_TYPE_MEM ) {
			nii->mem_bar = bar;
		} else if ( desc.acpi->ResType == ACPI_ADDRESS_SPACE_TYPE_IO ) {
			nii->io_bar = bar;
		}
		bs->FreePool ( desc.resource );
	}
	DBGC ( nii, "NII %s has ", nii->dev.name );
	if ( nii->mem_bar < PCI_MAX_BAR ) {
		DBGC ( nii, "memory BAR %d and ", nii->mem_bar );
	} else {
		DBGC ( nii, "no memory BAR and " );
	}
	if ( nii->io_bar < PCI_MAX_BAR ) {
		DBGC ( nii, "I/O BAR %d\n", nii->io_bar );
	} else {
		DBGC ( nii, "no I/O BAR\n" );
	}

	return 0;

 err_get_bar_attributes:
	efi_close_unsafe ( pci_device, &efi_pci_io_protocol_guid );
 err_open:
 err_locate:
	return rc;
}

/**
 * Close PCI I/O protocol
 *
 * @v nii		NII NIC
 * @ret rc		Return status code
 */
static void nii_pci_close ( struct nii_nic *nii ) {
	struct nii_mapping *map;
	struct nii_mapping *tmp;

	/* Remove any stale mappings */
	list_for_each_entry_safe ( map, tmp, &nii->mappings, list ) {
		DBGC ( nii, "NII %s removing stale mapping %#llx\n",
		       nii->dev.name, ( ( unsigned long long ) map->addr ) );
		nii->pci_io->Unmap ( nii->pci_io, map->mapping );
		list_del ( &map->list );
		free ( map );
	}

	/* Close protocols */
	efi_close_unsafe ( nii->pci_device, &efi_pci_io_protocol_guid );
}

/**
 * I/O callback
 *
 * @v unique_id		NII NIC
 * @v op		Operations
 * @v len		Length of data
 * @v addr		Address
 * @v data		Data buffer
 */
static EFIAPI VOID nii_io ( UINT64 unique_id, UINT8 op, UINT8 len, UINT64 addr,
			    UINT64 data ) {
	struct nii_nic *nii = ( ( void * ) ( intptr_t ) unique_id );
	EFI_PCI_IO_PROTOCOL_ACCESS *access;
	EFI_PCI_IO_PROTOCOL_IO_MEM io;
	EFI_PCI_IO_PROTOCOL_WIDTH width;
	unsigned int bar;
	EFI_STATUS efirc;
	int rc;

	/* Determine accessor and BAR */
	if ( op & ( PXE_MEM_READ | PXE_MEM_WRITE ) ) {
		access = &nii->pci_io->Mem;
		bar = nii->mem_bar;
	} else {
		access = &nii->pci_io->Io;
		bar = nii->io_bar;
	}

	/* Determine operaton */
	io = ( ( op & ( PXE_IO_WRITE | PXE_MEM_WRITE ) ) ?
	       access->Write : access->Read );

	/* Determine width */
	width = ( fls ( len ) - 1 );

	/* Issue operation */
	if ( ( efirc = io ( nii->pci_io, width, bar, addr, 1,
			    ( ( void * ) ( intptr_t ) data ) ) ) != 0 ) {
		rc = -EEFI ( efirc );
		DBGC ( nii, "NII %s I/O operation %#x failed: %s\n",
		       nii->dev.name, op, strerror ( rc ) );
		/* No way to report failure */
		return;
	}
}

/**
 * Map callback
 *
 * @v unique_id		NII NIC
 * @v addr		Address of memory to be mapped
 * @v len		Length of memory to be mapped
 * @v dir		Direction of data flow
 * @v mapped		Device mapped address to fill in
 */
static EFIAPI VOID nii_map ( UINT64 unique_id, UINT64 addr, UINT32 len,
			     UINT32 dir, UINT64 mapped ) {
	struct nii_nic *nii = ( ( void * ) ( intptr_t ) unique_id );
	EFI_PHYSICAL_ADDRESS *phys = ( ( void * ) ( intptr_t ) mapped );
	EFI_PCI_IO_PROTOCOL_OPERATION op;
	struct nii_mapping *map;
	UINTN count = len;
	EFI_STATUS efirc;
	int rc;

	/* Return a zero mapped address on failure */
	*phys = 0;

	/* Determine PCI mapping operation */
	switch ( dir ) {
	case TO_AND_FROM_DEVICE:
		op = EfiPciIoOperationBusMasterCommonBuffer;
		break;
	case FROM_DEVICE:
		op = EfiPciIoOperationBusMasterWrite;
		break;
	case TO_DEVICE:
		op = EfiPciIoOperationBusMasterRead;
		break;
	default:
		DBGC ( nii, "NII %s unsupported mapping direction %d\n",
		       nii->dev.name, dir );
		goto err_dir;
	}

	/* Allocate a mapping record */
	map = zalloc ( sizeof ( *map ) );
	if ( ! map )
		goto err_alloc;
	map->addr = addr;

	/* Create map */
	if ( ( efirc = nii->pci_io->Map ( nii->pci_io, op,
					  ( ( void * ) ( intptr_t ) addr ),
					  &count, phys, &map->mapping ) ) != 0){
		rc = -EEFI ( efirc );
		DBGC ( nii, "NII %s map operation failed: %s\n",
		       nii->dev.name, strerror ( rc ) );
		goto err_map;
	}

	/* Add to list of mappings */
	list_add ( &map->list, &nii->mappings );
	DBGC2 ( nii, "NII %s mapped %#llx+%#x->%#llx\n",
		nii->dev.name, ( ( unsigned long long ) addr ),
		len, ( ( unsigned long long ) *phys ) );
	return;

	list_del ( &map->list );
 err_map:
	free ( map );
 err_alloc:
 err_dir:
	return;
}

/**
 * Unmap callback
 *
 * @v unique_id		NII NIC
 * @v addr		Address of mapped memory
 * @v len		Length of mapped memory
 * @v dir		Direction of data flow
 * @v mapped		Device mapped address
 */
static EFIAPI VOID nii_unmap ( UINT64 unique_id, UINT64 addr, UINT32 len,
			       UINT32 dir __unused, UINT64 mapped ) {
	struct nii_nic *nii = ( ( void * ) ( intptr_t ) unique_id );
	struct nii_mapping *map;

	/* Locate mapping record */
	list_for_each_entry ( map, &nii->mappings, list ) {
		if ( map->addr == addr ) {
			nii->pci_io->Unmap ( nii->pci_io, map->mapping );
			list_del ( &map->list );
			free ( map );
			DBGC2 ( nii, "NII %s unmapped %#llx+%#x->%#llx\n",
				nii->dev.name, ( ( unsigned long long ) addr ),
				len, ( ( unsigned long long ) mapped ) );
			return;
		}
	}

	DBGC ( nii, "NII %s non-existent mapping %#llx+%#x->%#llx\n",
	       nii->dev.name, ( ( unsigned long long ) addr ),
	       len, ( ( unsigned long long ) mapped ) );
}

/**
 * Sync callback
 *
 * @v unique_id		NII NIC
 * @v addr		Address of mapped memory
 * @v len		Length of mapped memory
 * @v dir		Direction of data flow
 * @v mapped		Device mapped address
 */
static EFIAPI VOID nii_sync ( UINT64 unique_id __unused, UINT64 addr,
			      UINT32 len, UINT32 dir, UINT64 mapped ) {
	const void *src;
	void *dst;

	/* Do nothing if this is an identity mapping */
	if ( addr == mapped )
		return;

	/* Determine direction */
	if ( dir == FROM_DEVICE ) {
		src = ( ( void * ) ( intptr_t ) mapped );
		dst = ( ( void * ) ( intptr_t ) addr );
	} else {
		src = ( ( void * ) ( intptr_t ) addr );
		dst = ( ( void * ) ( intptr_t ) mapped );
	}

	/* Copy data */
	memcpy ( dst, src, len );
}

/**
 * Delay callback
 *
 * @v unique_id		NII NIC
 * @v microseconds	Delay in microseconds
 */
static EFIAPI VOID nii_delay ( UINT64 unique_id __unused, UINTN microseconds ) {

	udelay ( microseconds );
}

/**
 * Block callback
 *
 * @v unique_id		NII NIC
 * @v acquire		Acquire lock
 */
static EFIAPI VOID nii_block ( UINT64 unique_id, UINT32 acquire ) {
	struct nii_nic *nii = ( ( void * ) ( intptr_t ) unique_id );
	EFI_BOOT_SERVICES *bs = efi_systab->BootServices;

	/* This functionality (which is copied verbatim from the
	 * SnpDxe implementation of this function) appears to be
	 * totally brain-dead, since it produces no actual blocking
	 * behaviour.
	 */
	if ( acquire ) {
		nii->saved_tpl = bs->RaiseTPL ( TPL_NOTIFY );
	} else {
		bs->RestoreTPL ( nii->saved_tpl );
	}
}

/**
 * Construct operation from opcode and flags
 *
 * @v opcode		Opcode
 * @v opflags		Flags
 * @ret op		Operation
 */
#define NII_OP( opcode, opflags ) ( (opcode) | ( (opflags) << 16 ) )

/**
 * Extract opcode from operation
 *
 * @v op		Operation
 * @ret opcode		Opcode
 */
#define NII_OPCODE( op ) ( (op) & 0xffff )

/**
 * Extract flags from operation
 *
 * @v op		Operation
 * @ret opflags		Flags
 */
#define NII_OPFLAGS( op ) ( (op) >> 16 )

/**
 * Issue command with parameter block and data block
 *
 * @v nii		NII NIC
 * @v op		Operation
 * @v cpb		Command parameter block, or NULL
 * @v cpb_len		Command parameter block length
 * @v db		Data block, or NULL
 * @v db_len		Data block length
 * @ret stat		Status flags, or negative status code
 */
static int nii_issue_cpb_db ( struct nii_nic *nii, unsigned int op, void *cpb,
			      size_t cpb_len, void *db, size_t db_len ) {
	EFI_BOOT_SERVICES *bs = efi_systab->BootServices;
	PXE_CDB cdb;
	UINTN tpl;

	/* Prepare command descriptor block */
	memset ( &cdb, 0, sizeof ( cdb ) );
	cdb.OpCode = NII_OPCODE ( op );
	cdb.OpFlags = NII_OPFLAGS ( op );
	cdb.CPBaddr = ( ( intptr_t ) cpb );
	cdb.CPBsize = cpb_len;
	cdb.DBaddr = ( ( intptr_t ) db );
	cdb.DBsize = db_len;
	cdb.IFnum = nii->nii->IfNum;

	/* Raise task priority level */
	tpl = bs->RaiseTPL ( efi_internal_tpl );

	/* Issue command */
	DBGC2 ( nii, "NII %s issuing %02x:%04x ifnum %d%s%s\n",
		nii->dev.name, cdb.OpCode, cdb.OpFlags, cdb.IFnum,
		( cpb ? " cpb" : "" ), ( db ? " db" : "" ) );
	if ( cpb )
		DBGC2_HD ( nii, cpb, cpb_len );
	if ( db )
		DBGC2_HD ( nii, db, db_len );
	nii->issue ( ( intptr_t ) &cdb );

	/* Restore task priority level */
	bs->RestoreTPL ( tpl );

	/* Check completion status */
	if ( cdb.StatCode != PXE_STATCODE_SUCCESS )
		return -cdb.StatCode;

	/* Return command-specific status flags */
	return ( cdb.StatFlags & ~PXE_STATFLAGS_STATUS_MASK );
}

/**
 * Issue command with parameter block
 *
 * @v nii		NII NIC
 * @v op		Operation
 * @v cpb		Command parameter block, or NULL
 * @v cpb_len		Command parameter block length
 * @ret stat		Status flags, or negative status code
 */
static int nii_issue_cpb ( struct nii_nic *nii, unsigned int op, void *cpb,
			   size_t cpb_len ) {

	return nii_issue_cpb_db ( nii, op, cpb, cpb_len, NULL, 0 );
}

/**
 * Issue command with data block
 *
 * @v nii		NII NIC
 * @v op		Operation
 * @v db		Data block, or NULL
 * @v db_len		Data block length
 * @ret stat		Status flags, or negative status code
 */
static int nii_issue_db ( struct nii_nic *nii, unsigned int op, void *db,
			  size_t db_len ) {

	return nii_issue_cpb_db ( nii, op, NULL, 0, db, db_len );
}

/**
 * Issue command
 *
 *
 * @v nii		NII NIC
 * @v op		Operation
 * @ret stat		Status flags, or negative status code
 */
static int nii_issue ( struct nii_nic *nii, unsigned int op ) {

	return nii_issue_cpb_db ( nii, op, NULL, 0, NULL, 0 );
}

/**
 * Start UNDI
 *
 * @v nii		NII NIC
 * @ret rc		Return status code
 */
static int nii_start_undi ( struct nii_nic *nii ) {
	PXE_CPB_START_31 cpb;
	int stat;
	int rc;

	/* Construct parameter block */
	memset ( &cpb, 0, sizeof ( cpb ) );
	cpb.Delay = ( ( intptr_t ) nii_delay );
	cpb.Block = ( ( intptr_t ) nii_block );
	cpb.Mem_IO = ( ( intptr_t ) nii_io );
	cpb.Map_Mem = ( ( intptr_t ) nii_map );
	cpb.UnMap_Mem = ( ( intptr_t ) nii_unmap );
	cpb.Sync_Mem = ( ( intptr_t ) nii_sync );
	cpb.Unique_ID = ( ( intptr_t ) nii );

	/* Issue command */
	if ( ( stat = nii_issue_cpb ( nii, PXE_OPCODE_START, &cpb,
				      sizeof ( cpb ) ) ) < 0 ) {
		rc = -EIO_STAT ( stat );
		DBGC ( nii, "NII %s could not start: %s\n",
		       nii->dev.name, strerror ( rc ) );
		return rc;
	}

	return 0;
}

/**
 * Stop UNDI
 *
 * @v nii		NII NIC
 */
static void nii_stop_undi ( struct nii_nic *nii ) {
	int stat;
	int rc;

	/* Issue command */
	if ( ( stat = nii_issue ( nii, PXE_OPCODE_STOP ) ) < 0 ) {
		rc = -EIO_STAT ( stat );
		DBGC ( nii, "NII %s could not stop: %s\n",
		       nii->dev.name, strerror ( rc ) );
		/* Nothing we can do about it */
		return;
	}
}

/**
 * Get initialisation information
 *
 * @v nii		NII NIC
 * @v netdev		Network device to fill in
 * @ret rc		Return status code
 */
static int nii_get_init_info ( struct nii_nic *nii,
			       struct net_device *netdev ) {
	PXE_DB_GET_INIT_INFO db;
	int stat;
	int rc;

	/* Issue command */
	if ( ( stat = nii_issue_db ( nii, PXE_OPCODE_GET_INIT_INFO, &db,
				     sizeof ( db ) ) ) < 0 ) {
		rc = -EIO_STAT ( stat );
		DBGC ( nii, "NII %s could not get initialisation info: %s\n",
		       nii->dev.name, strerror ( rc ) );
		return rc;
	}

	/* Determine link layer protocol */
	switch ( db.IFtype ) {
	case PXE_IFTYPE_ETHERNET :
		netdev->ll_protocol = &ethernet_protocol;
		break;
	default:
		DBGC ( nii, "NII %s unknown interface type %#02x\n",
		       nii->dev.name, db.IFtype );
		return -ENOTSUP;
	}

	/* Sanity checks */
	assert ( db.MediaHeaderLen == netdev->ll_protocol->ll_header_len );
	assert ( db.HWaddrLen == netdev->ll_protocol->hw_addr_len );
	assert ( db.HWaddrLen == netdev->ll_protocol->ll_addr_len );

	/* Extract parameters */
	nii->buffer_len = db.MemoryRequired;
	nii->mtu = ( db.FrameDataLen + db.MediaHeaderLen );
	netdev->max_pkt_len = nii->mtu;
	nii->media = ( stat & PXE_STATFLAGS_GET_STATUS_NO_MEDIA_SUPPORTED );

	return 0;
}

/**
 * Initialise UNDI
 *
 * @v nii		NII NIC
 * @v flags		Flags
 * @ret rc		Return status code
 */
static int nii_initialise_flags ( struct nii_nic *nii, unsigned int flags ) {
	PXE_CPB_INITIALIZE cpb;
	PXE_DB_INITIALIZE db;
	unsigned int op;
	int stat;
	int rc;

	/* Allocate memory buffer */
	nii->buffer = umalloc ( nii->buffer_len );
	if ( ! nii->buffer ) {
		rc = -ENOMEM;
		goto err_alloc;
	}

	/* Construct parameter block */
	memset ( &cpb, 0, sizeof ( cpb ) );
	cpb.MemoryAddr = ( ( intptr_t ) nii->buffer );
	cpb.MemoryLength = nii->buffer_len;

	/* Construct data block */
	memset ( &db, 0, sizeof ( db ) );

	/* Issue command */
	op = NII_OP ( PXE_OPCODE_INITIALIZE, flags );
	if ( ( stat = nii_issue_cpb_db ( nii, op, &cpb, sizeof ( cpb ),
					 &db, sizeof ( db ) ) ) < 0 ) {
		rc = -EIO_STAT ( stat );
		DBGC ( nii, "NII %s could not initialise: %s\n",
		       nii->dev.name, strerror ( rc ) );
		goto err_initialize;
	}

	return 0;

 err_initialize:
	ufree ( nii->buffer );
 err_alloc:
	return rc;
}

/**
 * Initialise UNDI with cable detection
 *
 * @v nii		NII NIC
 * @ret rc		Return status code
 */
static int nii_initialise_cable ( struct nii_nic *nii ) {
	unsigned int flags;

	/* Initialise UNDI */
	flags = PXE_OPFLAGS_INITIALIZE_DETECT_CABLE;
	return nii_initialise_flags ( nii, flags );
}

/**
 * Initialise UNDI
 *
 * @v nii		NII NIC
 * @ret rc		Return status code
 */
static int nii_initialise ( struct nii_nic *nii ) {
	unsigned int flags;

	/* Initialise UNDI */
	flags = PXE_OPFLAGS_INITIALIZE_DO_NOT_DETECT_CABLE;
	return nii_initialise_flags ( nii, flags );
}

/**
 * Shut down UNDI
 *
 * @v nii		NII NIC
 */
static void nii_shutdown ( struct nii_nic *nii ) {
	int stat;
	int rc;

	/* Issue command */
	if ( ( stat = nii_issue ( nii, PXE_OPCODE_SHUTDOWN ) ) < 0 ) {
		rc = -EIO_STAT ( stat );
		DBGC ( nii, "NII %s could not shut down: %s\n",
		       nii->dev.name, strerror ( rc ) );
		/* Leak memory to avoid corruption */
		return;
	}

	/* Free buffer */
	ufree ( nii->buffer );
}

/**
 * Get station addresses
 *
 * @v nii		NII NIC
 * @v netdev		Network device to fill in
 * @ret rc		Return status code
 */
static int nii_get_station_address ( struct nii_nic *nii,
				     struct net_device *netdev ) {
	PXE_DB_STATION_ADDRESS db;
	unsigned int op;
	int stat;
	int rc;

	/* Initialise UNDI */
	if ( ( rc = nii_initialise ( nii ) ) != 0 )
		goto err_initialise;

	/* Issue command */
	op = NII_OP ( PXE_OPCODE_STATION_ADDRESS,
		      PXE_OPFLAGS_STATION_ADDRESS_READ );
	if ( ( stat = nii_issue_db ( nii, op, &db, sizeof ( db ) ) ) < 0 ) {
		rc = -EIO_STAT ( stat );
		DBGC ( nii, "NII %s could not get station address: %s\n",
		       nii->dev.name, strerror ( rc ) );
		goto err_station_address;
	}

	/* Copy MAC addresses */
	memcpy ( netdev->ll_addr, db.StationAddr,
		 netdev->ll_protocol->ll_addr_len );
	memcpy ( netdev->hw_addr, db.PermanentAddr,
		 netdev->ll_protocol->hw_addr_len );
	memcpy ( nii->broadcast, db.BroadcastAddr,
		 sizeof ( nii->broadcast ) );

 err_station_address:
	nii_shutdown ( nii );
 err_initialise:
	return rc;
}

/**
 * Set station address
 *
 * @v nii		NII NIC
 * @v netdev		Network device
 * @ret rc		Return status code
 */
static int nii_set_station_address ( struct nii_nic *nii,
				     struct net_device *netdev ) {
	uint32_t implementation = nii->undi->Implementation;
	PXE_CPB_STATION_ADDRESS cpb;
	unsigned int op;
	int stat;
	int rc;

	/* Fail if setting station address is unsupported */
	if ( ! ( implementation & PXE_ROMID_IMP_STATION_ADDR_SETTABLE ) )
		return -ENOTSUP;

	/* Construct parameter block */
	memset ( &cpb, 0, sizeof ( cpb ) );
	memcpy ( cpb.StationAddr, netdev->ll_addr,
		 netdev->ll_protocol->ll_addr_len );

	/* Issue command */
	op = NII_OP ( PXE_OPCODE_STATION_ADDRESS,
	              PXE_OPFLAGS_STATION_ADDRESS_WRITE );
	if ( ( stat = nii_issue_cpb ( nii, op, &cpb, sizeof ( cpb ) ) ) < 0 ) {
		rc = -EIO_STAT ( stat );
		DBGC ( nii, "NII %s could not set station address: %s\n",
		       nii->dev.name, strerror ( rc ) );
		return rc;
	}

	return 0;
}

/**
 * Set receive filters
 *
 * @v nii		NII NIC
 * @v flags		Flags
 * @ret rc		Return status code
 */
static int nii_set_rx_filters ( struct nii_nic *nii, unsigned int flags ) {
	uint32_t implementation = nii->undi->Implementation;
	unsigned int op;
	int stat;
	int rc;

	/* Construct receive filter set */
	flags |= PXE_OPFLAGS_RECEIVE_FILTER_UNICAST;
	if ( implementation & PXE_ROMID_IMP_BROADCAST_RX_SUPPORTED )
		flags |= PXE_OPFLAGS_RECEIVE_FILTER_BROADCAST;
	if ( implementation & PXE_ROMID_IMP_PROMISCUOUS_RX_SUPPORTED )
		flags |= PXE_OPFLAGS_RECEIVE_FILTER_PROMISCUOUS;
	if ( implementation & PXE_ROMID_IMP_PROMISCUOUS_MULTICAST_RX_SUPPORTED )
		flags |= PXE_OPFLAGS_RECEIVE_FILTER_ALL_MULTICAST;

	/* Issue command */
	op = NII_OP ( PXE_OPCODE_RECEIVE_FILTERS, flags );
	if ( ( stat = nii_issue ( nii, op ) ) < 0 ) {
		rc = -EIO_STAT ( stat );
		DBGC ( nii, "NII %s could not %s%sable receive filters "
		       "%#04x: %s\n", nii->dev.name,
		       ( ( flags & PXE_OPFLAGS_RECEIVE_FILTER_ENABLE ) ?
			 "en" : "" ),
		       ( ( flags & PXE_OPFLAGS_RECEIVE_FILTER_DISABLE ) ?
			 "dis" : "" ), flags, strerror ( rc ) );
		return rc;
	}

	return 0;
}

/**
 * Enable receive filters
 *
 * @v nii		NII NIC
 * @ret rc		Return status code
 */
static int nii_enable_rx_filters ( struct nii_nic *nii ) {

	return nii_set_rx_filters ( nii, PXE_OPFLAGS_RECEIVE_FILTER_ENABLE );
}

/**
 * Disable receive filters
 *
 * @v nii		NII NIC
 * @ret rc		Return status code
 */
static int nii_disable_rx_filters ( struct nii_nic *nii ) {

	return nii_set_rx_filters ( nii, PXE_OPFLAGS_RECEIVE_FILTER_DISABLE );
}

/**
 * Transmit packet
 *
 * @v netdev		Network device
 * @v iobuf		I/O buffer
 * @ret rc		Return status code
 */
static int nii_transmit ( struct net_device *netdev,
			  struct io_buffer *iobuf ) {
	struct nii_nic *nii = netdev->priv;
	PXE_CPB_TRANSMIT cpb;
	unsigned int op;
	int stat;
	int rc;

	/* Defer the packet if there is already a transmission in progress */
	if ( nii->txbuf ) {
		netdev_tx_defer ( netdev, iobuf );
		return 0;
	}

	/* Pad to minimum Ethernet length, to work around underlying
	 * drivers that do not correctly handle frame padding
	 * themselves.
	 */
	iob_pad ( iobuf, ETH_ZLEN );

	/* Construct parameter block */
	memset ( &cpb, 0, sizeof ( cpb ) );
	cpb.FrameAddr = ( ( intptr_t ) iobuf->data );
	cpb.DataLen = iob_len ( iobuf );

	/* Transmit packet */
	op = NII_OP ( PXE_OPCODE_TRANSMIT,
		      ( PXE_OPFLAGS_TRANSMIT_WHOLE |
			PXE_OPFLAGS_TRANSMIT_DONT_BLOCK ) );
	if ( ( stat = nii_issue_cpb ( nii, op, &cpb, sizeof ( cpb ) ) ) < 0 ) {
		rc = -EIO_STAT ( stat );
		DBGC ( nii, "NII %s could not transmit: %s\n",
		       nii->dev.name, strerror ( rc ) );
		return rc;
	}
	nii->txbuf = iobuf;

	return 0;
}

/**
 * Poll for completed packets
 *
 * @v netdev		Network device
 * @v stat		Status flags
 */
static void nii_poll_tx ( struct net_device *netdev, unsigned int stat ) {
	struct nii_nic *nii = netdev->priv;
	struct io_buffer *iobuf;

	/* Do nothing unless we have a completion */
	if ( stat & PXE_STATFLAGS_GET_STATUS_NO_TXBUFS_WRITTEN )
		return;

	/* Ignore spurious completions reported by some devices */
	if ( ! nii->txbuf )
		return;

	/* Complete transmission */
	iobuf = nii->txbuf;
	nii->txbuf = NULL;
	netdev_tx_complete ( netdev, iobuf );
}

/**
 * Poll for received packets
 *
 * @v netdev		Network device
 */
static void nii_poll_rx ( struct net_device *netdev ) {
	struct nii_nic *nii = netdev->priv;
	PXE_CPB_RECEIVE cpb;
	PXE_DB_RECEIVE db;
	unsigned int quota;
	int stat;
	int rc;

	/* Retrieve up to NII_RX_QUOTA packets */
	for ( quota = NII_RX_QUOTA ; quota ; quota-- ) {

		/* Allocate buffer, if required */
		if ( ! nii->rxbuf ) {
			nii->rxbuf = alloc_iob ( nii->mtu );
			if ( ! nii->rxbuf ) {
				/* Leave for next poll */
				break;
			}
		}

		/* Construct parameter block */
		memset ( &cpb, 0, sizeof ( cpb ) );
		cpb.BufferAddr = ( ( intptr_t ) nii->rxbuf->data );
		cpb.BufferLen = iob_tailroom ( nii->rxbuf );

		/* Issue command */
		if ( ( stat = nii_issue_cpb_db ( nii, PXE_OPCODE_RECEIVE,
						 &cpb, sizeof ( cpb ),
						 &db, sizeof ( db ) ) ) < 0 ) {

			/* PXE_STATCODE_NO_DATA is just the usual "no packet"
			 * status indicator; ignore it.
			 */
			if ( stat == -PXE_STATCODE_NO_DATA )
				break;

			/* Anything else is an error */
			rc = -EIO_STAT ( stat );
			DBGC ( nii, "NII %s could not receive: %s\n",
			       nii->dev.name, strerror ( rc ) );
			netdev_rx_err ( netdev, NULL, rc );
			break;
		}

		/* Hand off to network stack */
		iob_put ( nii->rxbuf, db.FrameLen );
		netdev_rx ( netdev, nii->rxbuf );
		nii->rxbuf = NULL;
	}
}

/**
 * Check for link state changes
 *
 * @v netdev		Network device
 * @v stat		Status flags
 */
static void nii_poll_link ( struct net_device *netdev, unsigned int stat ) {
	int no_media = ( stat & PXE_STATFLAGS_GET_STATUS_NO_MEDIA );

	if ( no_media && netdev_link_ok ( netdev ) ) {
		netdev_link_down ( netdev );
	} else if ( ( ! no_media ) && ( ! netdev_link_ok ( netdev ) ) ) {
		netdev_link_up ( netdev );
	}
}

/**
 * Poll for completed packets
 *
 * @v netdev		Network device
 */
static void nii_poll ( struct net_device *netdev ) {
	struct nii_nic *nii = netdev->priv;
	PXE_DB_GET_STATUS db;
	unsigned int op;
	int stat;
	int rc;

	/* Construct data block */
	memset ( &db, 0, sizeof ( db ) );

	/* Get status */
	op = NII_OP ( PXE_OPCODE_GET_STATUS,
		      ( PXE_OPFLAGS_GET_INTERRUPT_STATUS |
			PXE_OPFLAGS_GET_TRANSMITTED_BUFFERS |
			( nii->media ? PXE_OPFLAGS_GET_MEDIA_STATUS : 0 ) ) );
	if ( ( stat = nii_issue_db ( nii, op, &db, sizeof ( db ) ) ) < 0 ) {
		rc = -EIO_STAT ( stat );
		DBGC ( nii, "NII %s could not get status: %s\n",
		       nii->dev.name, strerror ( rc ) );
		return;
	}

	/* Process any TX completions */
	nii_poll_tx ( netdev, stat );

	/* Process any RX completions */
	nii_poll_rx ( netdev );

	/* Check for link state changes */
	if ( nii->media )
		nii_poll_link ( netdev, stat );
}

/**
 * Open network device
 *
 * @v netdev		Network device
 * @ret rc		Return status code
 */
static int nii_open ( struct net_device *netdev ) {
	struct nii_nic *nii = netdev->priv;
	int rc;

	/* Initialise NIC
	 *
	 * We don't care about link state here, and would prefer to
	 * have the NIC initialise even if no cable is present, to
	 * match the behaviour of all other iPXE drivers.
	 *
	 * Some Emulex NII drivers have a bug which prevents packets
	 * from being sent or received unless we specifically ask it
	 * to detect cable presence during initialisation.
	 *
	 * Unfortunately, some other NII drivers (e.g. Mellanox) may
	 * time out and report failure if asked to detect cable
	 * presence during initialisation on links that are physically
	 * slow to reach link-up.
	 *
	 * Attempt to work around both of these problems by first
	 * attempting to initialise with cable presence detection,
	 * then falling back to initialising without cable presence
	 * detection.
	 */
	if ( ( rc = nii_initialise_cable ( nii ) ) != 0 ) {
		DBGC ( nii, "NII %s could not initialise with cable "
		       "detection: %s\n", nii->dev.name, strerror ( rc ) );
		if ( ( rc = nii_initialise ( nii ) ) != 0 ) {
			DBGC ( nii, "NII %s could not initialise without "
			       "cable detection: %s\n",
			       nii->dev.name, strerror ( rc ) );
			goto err_initialise;
		}
	}

	/* Attempt to set station address */
	if ( ( rc = nii_set_station_address ( nii, netdev ) ) != 0 ) {
		DBGC ( nii, "NII %s could not set station address: %s\n",
		       nii->dev.name, strerror ( rc ) );
		/* Treat as non-fatal */
	}

	/* Disable receive filters
	 *
	 * We have no reason to disable receive filters here (or
	 * anywhere), but some NII drivers have a bug which prevents
	 * packets from being received unless we attempt to disable
	 * the receive filters.
	 *
	 * Ignore any failures, since we genuinely don't care if the
	 * NII driver cannot disable the filters.
	 */
	nii_disable_rx_filters ( nii );

	/* Enable receive filters */
	if ( ( rc = nii_enable_rx_filters ( nii ) ) != 0 )
		goto err_enable_rx_filters;

	return 0;

 err_enable_rx_filters:
	nii_shutdown ( nii );
 err_initialise:
	return rc;
}

/**
 * Close network device
 *
 * @v netdev		Network device
 */
static void nii_close ( struct net_device *netdev ) {
	struct nii_nic *nii = netdev->priv;

	/* Shut down NIC */
	nii_shutdown ( nii );

	/* Discard transmit buffer, if applicable */
	if ( nii->txbuf ) {
		netdev_tx_complete_err ( netdev, nii->txbuf, -ECANCELED );
		nii->txbuf = NULL;
	}

	/* Discard receive buffer, if applicable */
	if ( nii->rxbuf ) {
		free_iob ( nii->rxbuf );
		nii->rxbuf = NULL;
	}
}

/** NII network device operations */
static struct net_device_operations nii_operations = {
	.open = nii_open,
	.close = nii_close,
	.transmit = nii_transmit,
	.poll = nii_poll,
};

/**
 * Attach driver to device
 *
 * @v efidev		EFI device
 * @ret rc		Return status code
 */
int nii_start ( struct efi_device *efidev ) {
	EFI_HANDLE device = efidev->device;
	struct net_device *netdev;
	struct nii_nic *nii;
	int rc;

	/* Allocate and initialise structure */
	netdev = alloc_netdev ( sizeof ( *nii ) );
	if ( ! netdev ) {
		rc = -ENOMEM;
		goto err_alloc;
	}
	netdev_init ( netdev, &nii_operations );
	nii = netdev->priv;
	nii->efidev = efidev;
	INIT_LIST_HEAD ( &nii->mappings );
	netdev->ll_broadcast = nii->broadcast;
	efidev_set_drvdata ( efidev, netdev );

	/* Populate underlying device information */
	efi_device_info ( device, "NII", &nii->dev );
	nii->dev.driver_name = "NII";
	nii->dev.parent = &efidev->dev;
	list_add ( &nii->dev.siblings, &efidev->dev.children );
	INIT_LIST_HEAD ( &nii->dev.children );
	netdev->dev = &nii->dev;

	/* Open NII protocol */
	if ( ( rc = efi_open_by_driver ( device, &efi_nii31_protocol_guid,
					 &nii->nii ) ) != 0 ) {
		DBGC ( nii, "NII %s cannot open NII protocol: %s\n",
		       nii->dev.name, strerror ( rc ) );
		DBGC_EFI_OPENERS ( device, device, &efi_nii31_protocol_guid );
		goto err_open_protocol;
	}

	/* Locate UNDI and entry point */
	nii->undi = ( ( void * ) ( intptr_t ) nii->nii->Id );
	if ( ! nii->undi ) {
		DBGC ( nii, "NII %s has no UNDI\n", nii->dev.name );
		rc = -ENODEV;
		goto err_no_undi;
	}
	if ( nii->undi->Implementation & PXE_ROMID_IMP_HW_UNDI ) {
		DBGC ( nii, "NII %s is a mythical hardware UNDI\n",
		       nii->dev.name );
		rc = -ENOTSUP;
		goto err_hw_undi;
	}
	if ( nii->undi->Implementation & PXE_ROMID_IMP_SW_VIRT_ADDR ) {
		nii->issue = ( ( void * ) ( intptr_t ) nii->undi->EntryPoint );
	} else {
		nii->issue = ( ( ( void * ) nii->undi ) +
			       nii->undi->EntryPoint );
	}
	DBGC ( nii, "NII %s using UNDI v%x.%x at %p entry %p impl %#08x\n",
	       nii->dev.name, nii->nii->MajorVer, nii->nii->MinorVer,
	       nii->undi, nii->issue, nii->undi->Implementation );

	/* Open PCI I/O protocols and locate BARs */
	if ( ( rc = nii_pci_open ( nii ) ) != 0 )
		goto err_pci_open;

	/* Start UNDI */
	if ( ( rc = nii_start_undi ( nii ) ) != 0 )
		goto err_start_undi;

	/* Get initialisation information */
	if ( ( rc = nii_get_init_info ( nii, netdev ) ) != 0 )
		goto err_get_init_info;

	/* Get MAC addresses */
	if ( ( rc = nii_get_station_address ( nii, netdev ) ) != 0 )
		goto err_get_station_address;

	/* Register network device */
	if ( ( rc = register_netdev ( netdev ) ) != 0 )
		goto err_register_netdev;
	DBGC ( nii, "NII %s registered as %s for %s\n", nii->dev.name,
	       netdev->name, efi_handle_name ( device ) );

	/* Set initial link state (if media detection is not supported) */
	if ( ! nii->media )
		netdev_link_up ( netdev );

	return 0;

	unregister_netdev ( netdev );
 err_register_netdev:
 err_get_station_address:
 err_get_init_info:
	nii_stop_undi ( nii );
 err_start_undi:
	nii_pci_close ( nii );
 err_pci_open:
 err_hw_undi:
 err_no_undi:
	efi_close_by_driver ( device, &efi_nii31_protocol_guid );
 err_open_protocol:
	list_del ( &nii->dev.siblings );
	netdev_nullify ( netdev );
	netdev_put ( netdev );
 err_alloc:
	return rc;
}

/**
 * Detach driver from device
 *
 * @v efidev		EFI device
 */
void nii_stop ( struct efi_device *efidev ) {
	struct net_device *netdev = efidev_get_drvdata ( efidev );
	struct nii_nic *nii = netdev->priv;
	EFI_HANDLE device = efidev->device;

	/* Unregister network device */
	unregister_netdev ( netdev );

	/* Stop UNDI */
	nii_stop_undi ( nii );

	/* Close PCI I/O protocols */
	nii_pci_close ( nii );

	/* Close NII protocol */
	efi_close_by_driver ( device, &efi_nii31_protocol_guid );

	/* Free network device */
	list_del ( &nii->dev.siblings );
	netdev_nullify ( netdev );
	netdev_put ( netdev );
}
