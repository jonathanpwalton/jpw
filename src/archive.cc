#include "jpw.h"

#include <archive.h>
#include <archive_entry.h>

static inline void require(bool predicate, jpw::string const & msg) {
	if (!predicate) throw std::runtime_error(msg);
}

void jpw::extract(File & file, Path const & dir) {
	auto cwd = fs::current_path();
	fs::current_path(dir);

	struct archive * archive = archive_read_new();
	require(archive != nullptr, "failed to create new read archive");
	require(archive_read_support_format_all(archive) == ARCHIVE_OK, "failed to support all archive formats");
	require(archive_read_support_filter_all(archive) == ARCHIVE_OK, "failed to support all archive filters");

	if (file.buffer) require(archive_read_open_memory(archive, file.buffer, file.length) == ARCHIVE_OK, "failed to read archive from memory");
	else require(archive_read_open_FILE(archive, file) == ARCHIVE_OK, "failed to read archive from file");

	struct archive * ext = archive_write_disk_new();
	require(ext != nullptr, "failed to create new write archive");
	require(archive_write_disk_set_options(ext, ARCHIVE_EXTRACT_ACL | ARCHIVE_EXTRACT_TIME | ARCHIVE_EXTRACT_PERM | ARCHIVE_EXTRACT_FFLAGS) == ARCHIVE_OK, "failed to set write archive flags");
	require(archive_write_disk_set_standard_lookup(ext) == ARCHIVE_OK, "failed to set write archive standard lookup");

	int status;
	struct archive_entry * entry;
	while (true) {
		status = archive_read_next_header(archive, &entry);
		while (status == ARCHIVE_RETRY) archive_read_next_header(archive, &entry);

		if (status == ARCHIVE_EOF) break;
		else if (status < ARCHIVE_WARN) throw std::runtime_error(string("failed to read archive header (") + archive_error_string(archive) + ")");
		else if (status < ARCHIVE_OK) printf("%*s\033[0m\033[1;93mwarning: \033[0m%s\n", indent, "", archive_error_string(archive));

		status = archive_write_header(ext, entry);
		while (status == ARCHIVE_RETRY) status = archive_write_header(ext, entry);

		if (status < ARCHIVE_WARN) throw std::runtime_error(string("failed to write archive header (") + archive_error_string(ext) + ")");
		else if (status < ARCHIVE_OK) printf("%*s\033[0m\033[1;93mwarning: \033[0m%s\n", indent, "", archive_error_string(ext));

		while (true) {
			void const * buffer;
			size_t length;
			la_int64_t offset;

			status = archive_read_data_block(archive, &buffer, &length, &offset);

			if (status == ARCHIVE_EOF) break;
			else if (status < ARCHIVE_WARN) throw std::runtime_error(string("failed to read archive data block (") + archive_error_string(archive) + ")");
			else if (status < ARCHIVE_OK) printf("%*s\033[0m\033[1;93mwarning: \033[0m%s\n", indent, "", archive_error_string(archive));

			if (archive_write_data_block(ext, buffer, length, offset) < 0) throw std::runtime_error(string("failed to write archive data block (") + archive_error_string(ext) + ")");
		}

		status = archive_write_finish_entry(ext);
		while (status == ARCHIVE_RETRY) status = archive_write_finish_entry(ext);

		if (status < ARCHIVE_WARN) throw std::runtime_error(string("failed to write archive entry (") + archive_error_string(ext) + ")");
		else if (status < ARCHIVE_OK) printf("%*s\033[0m\033[1;93mwarning: \033[0m%s\n", indent, "", archive_error_string(ext));
	}

	require(archive_read_close(archive) == ARCHIVE_OK && archive_write_close(ext) == ARCHIVE_OK, "failed to close archive");
	require(archive_read_free(archive) == ARCHIVE_OK && archive_write_free(ext) == ARCHIVE_OK, "failed to free archive");
	fs::current_path(cwd);
}
