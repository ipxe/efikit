/*
 * Copyright (C) 2020 Michael Brown <mbrown@fensystems.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 * EFI boot device command-line tool
 *
 */

#ifndef _EFIBOOTCLI_H
#define _EFIBOOTCLI_H

struct efi_boot_command;

extern struct efi_boot_command efibootshow;
extern struct efi_boot_command efibootmod;
extern struct efi_boot_command efibootadd;
extern struct efi_boot_command efibootdel;

extern int efiboot_command ( int argc, char **argv,
			     struct efi_boot_command *cmd );

#endif /* _EFIBOOTCLI_H */
