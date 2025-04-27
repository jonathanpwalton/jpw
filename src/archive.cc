#include "core.h"
#include <archive.h>
#include <archive_entry.h>

struct ReadArchive {
	struct archive * self;
	bool complete = false;

	ReadArchive(void const * buffer, size_t size) : self(archive_read_new()) {
		self != NULL &&
		archive_read_support_format_all(self) == ARCHIVE_OK &&
		archive_read_support_filter_all(self) == ARCHIVE_OK &&
		archive_read_open_memory(self, buffer, size) == ARCHIVE_OK &&
		(complete = true);
	}

	~ReadArchive() { self != NULL && archive_read_free(self);	}
	operator struct archive * () { return self; }
};

struct WriteArchive {
	struct archive * self;
	bool complete = false;

	WriteArchive(int flags = ARCHIVE_EXTRACT_TIME | ARCHIVE_EXTRACT_PERM | ARCHIVE_EXTRACT_ACL | ARCHIVE_EXTRACT_FFLAGS) : self(archive_write_disk_new()) {
		self != NULL &&
		archive_write_disk_set_options(self, ARCHIVE_EXTRACT_TIME | ARCHIVE_EXTRACT_PERM | ARCHIVE_EXTRACT_ACL | ARCHIVE_EXTRACT_FFLAGS) == ARCHIVE_OK &&
		archive_write_disk_set_standard_lookup(self) == ARCHIVE_OK &&
		(complete = true);
	}

	~WriteArchive() {	self != NULL && archive_write_free(self);	}
	operator struct archive * () { return self; }
};

bool jpw::extract_archive(IO & src, Path dst) {
	list<uint8_t> bytes = src.read();
	if (bytes.empty()) return false;

	ReadArchive read(bytes.data(), bytes.size());
	if (!read.complete) return false;

	WriteArchive write;
	if (!write.complete) return false;

	auto cwd = fs::current_path();
	fs::current_path(dst);

	struct archive_entry * entry; int r;
	while (true) {
		if ((r = archive_read_next_header(read, &entry)) == ARCHIVE_EOF)
			break;
		else if (r < ARCHIVE_WARN)
			return false;
		if ((r = archive_write_header(write, entry)) < ARCHIVE_WARN)
			return false;
		else if (archive_entry_size(entry) > 0) {
			const void * buffer; size_t size;
			la_int64_t offset;

			while (true) {
				if ((r = archive_read_data_block(read, &buffer, &size, &offset)) == ARCHIVE_EOF)
					break;
				else if (r < ARCHIVE_WARN)
					return false;
				if (archive_write_data_block(write, buffer, size, offset) < ARCHIVE_WARN)
					return false;
			}
		}
		if (archive_write_finish_entry(write) < ARCHIVE_WARN)
			return false;
	}

	fs::current_path(cwd);
	return true;
}