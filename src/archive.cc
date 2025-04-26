#include "core.h"
#include <archive.h>
#include <archive_entry.h>

bool jpw::extract_archive(IO & src, Path dst) {
	list<uint8_t> bytes = src.read();
	if (bytes.empty()) return false;

	struct archive * a = archive_read_new();
	if (!a) return false;
	if (archive_read_support_format_all(a) != ARCHIVE_OK) return false;
	if (archive_read_support_filter_all(a) != ARCHIVE_OK) return false;
	if (archive_read_open_memory(a, bytes.data(), bytes.size()) != ARCHIVE_OK) return false;

	struct archive * e = archive_write_disk_new();
	if (!e) return false;
	if (archive_write_disk_set_options(e, ARCHIVE_EXTRACT_TIME | ARCHIVE_EXTRACT_PERM | ARCHIVE_EXTRACT_ACL | ARCHIVE_EXTRACT_FFLAGS) != ARCHIVE_OK) return false;
	if (archive_write_disk_set_standard_lookup(e) != ARCHIVE_OK) return false;

	auto cwd = fs::current_path();
	fs::current_path(dst);

	struct archive_entry * entry; int r;
	while (true) {
		if ((r = archive_read_next_header(a, &entry)) == ARCHIVE_EOF)
			break;
		else if (r < ARCHIVE_WARN)
			return false;
		if ((r = archive_write_header(e, entry)) < ARCHIVE_WARN)
			return false;
		else if (archive_entry_size(entry) > 0) {
			const void * buffer; size_t size;
			la_int64_t offset;

			while (true) {
				if ((r = archive_read_data_block(a, &buffer, &size, &offset)) == ARCHIVE_EOF)
					break;
				else if (r < ARCHIVE_WARN)
					return false;
				if (archive_write_data_block(e, buffer, size, offset) < ARCHIVE_WARN)
					return false;
			}
		}
		if (archive_write_finish_entry(e) < ARCHIVE_WARN)
			return false;
	}

	archive_read_close(a);
	archive_read_free(a);

	archive_write_close(e);
	archive_write_free(e);

	fs::current_path(cwd);
	return true;
}