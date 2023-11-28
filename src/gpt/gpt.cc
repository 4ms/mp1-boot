extern "C" {
#include "crc32.h"
}
#include "compiler.h"
#include "part_efi.h"
#include "print_messages.hh"

u32 efi_crc32(const void *buf, u32 len) { return crc32(0, (uint8_t *)buf, len); }

bool validate_gpt_header(gpt_header *gpt_h, lbaint_t lba, lbaint_t lastlba)
{
	uint32_t crc32_backup = 0;
	uint32_t calc_crc32;

	/* Check the GPT header signature */
	if (le64_to_cpu(gpt_h->signature) != GPT_HEADER_SIGNATURE_UBOOT) {
		pr_err("GUID Partition Table Header signature is wrong: ",
			   Hex(le64_to_cpu(gpt_h->signature) >> 32),
			   " != ",
			   Hex{GPT_HEADER_SIGNATURE_UBOOT >> 32},
			   "\n");
		return false;
	}

	/* Check the GUID Partition Table CRC */
	crc32_backup = gpt_h->header_crc32;
	gpt_h->header_crc32 = 0;
	calc_crc32 = efi_crc32((const unsigned char *)gpt_h, le32_to_cpu(gpt_h->header_size));
	gpt_h->header_crc32 = crc32_backup;
	if (calc_crc32 != le32_to_cpu(crc32_backup)) {
		pr_err("GUID Partition Table Header CRC is wrong: ", le32_to_cpu(crc32_backup), " != ", calc_crc32, "\n");
		return false;
	}

	/*
	 * Check that the my_lba entry points to the LBA that contains the GPT
	 */
	if (le64_to_cpu(gpt_h->my_lba) != lba) {
		pr_err("GPT: my_lba incorrect: ", le64_to_cpu(gpt_h->my_lba), " != ", lba, "\n");
		return false;
	}

	/*
	 * Check that the first_usable_lba and that the last_usable_lba are
	 * within the disk. Also partition_entry_lba (DG)
	 */
	if (le64_to_cpu(gpt_h->first_usable_lba) > lastlba) {
		pr_err("GPT: first_usable_lba incorrect: ", le64_to_cpu(gpt_h->first_usable_lba), " != ", lastlba, "\n");
		return false;
	}
	if (le64_to_cpu(gpt_h->last_usable_lba) > lastlba) {
		pr_err("GPT: last_usable_lba incorrect: ", le64_to_cpu(gpt_h->last_usable_lba), " != ", lastlba, "\n");
		return false;
	}
	if (le64_to_cpu(gpt_h->partition_entry_lba) > lastlba) {
		pr_err("GPT: partition_entry_lba incorrect: ", le64_to_cpu(gpt_h->partition_entry_lba), " != ", lastlba, "\n");
		return false;
	}

	if (le64_to_cpu(gpt_h->signature) == GPT_HEADER_CHROMEOS_IGNORE) {
		pr_err("ChromeOS 'IGNOREME' GPT header found and ignored\n");
		return false;
	}

	log("GPT: first_usable_lba: ",
		le64_to_cpu(gpt_h->first_usable_lba),
		" last_usable_lba: ",
		le64_to_cpu(gpt_h->last_usable_lba),
		" last lba: ",
		lastlba,
		"\n");

	log("Raw header:", "\n");
	log("signature: ", gpt_h->signature, "\n");				  //__le64
	log("revision: ", gpt_h->revision, "\n");				  //__le32
	log("header_size: ", gpt_h->header_size, "\n");			  //__le32
	log("header_crc32: ", gpt_h->header_crc32, "\n");		  //__le32
	log("reserved1: ", gpt_h->reserved1, "\n");				  //__le32
	log("my_lba: ", gpt_h->my_lba, "\n");					  //__le64
	log("alternate_lba: ", gpt_h->alternate_lba, "\n");		  //__le64
	log("first_usable_lba: ", gpt_h->first_usable_lba, "\n"); //__le64
	log("last_usable_lba: ", gpt_h->last_usable_lba, "\n");	  //__le64
	// log("disk_guid: ", gpt_h->disk_guid, "\n");										// efi_guid_t
	log("partition_entry_lba: ", gpt_h->partition_entry_lba, "\n");				   //__le64
	log("num_partition_entries: ", gpt_h->num_partition_entries, "\n");			   //__le32
	log("sizeof_partition_entry: ", gpt_h->sizeof_partition_entry, "\n");		   //__le32
	log("partition_entry_array_crc32:", gpt_h->partition_entry_array_crc32, "\n"); //__le32

	return true;
}

bool validate_partition_entry(const gpt_entry &pte)
{
	bool valid_guid = false;
	for (unsigned i = 0; i < sizeof(pte.partition_type_guid.b); i++) {
		if (pte.partition_type_guid.b[i] != 0)
			valid_guid = true;
	}
	if (!valid_guid) {
		log("Invalid GUID (all zeroes) found on partition entry. Skipping\n");
		return false;
	}

	log("partition_type_guid[0]: ", pte.partition_type_guid.b[0], "\n");  // efi_guid_t
	log("unique_partition_guid: ", pte.unique_partition_guid.b[0], "\n"); // efi_guid_t
	log("starting_lba: ", pte.starting_lba, "\n");						  //__le64
	log("ending_lba: ", pte.ending_lba, "\n");							  //__le64
	log("attributes: ", pte.attributes.raw, "\n");						  // gpt_entry_attributes
	// log("partition_name[PARTNAME_SZ]: %36s\n", (char *)pte.partition_name); // efi_char16_t
	return true;
}
