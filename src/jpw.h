#pragma once

#include <string>
#include <vector>
#include <queue>
#include <memory>
#include <filesystem>

namespace std::filesystem {

	size_t entry_count(path const & dir);

}

namespace jpw {

	namespace fs = std::filesystem;

	using std::string, std::vector, std::queue;
	using Path = fs::path;

	struct ProgramError : public std::exception {
		string msg;
		ProgramError(string const & msg) : msg(msg) {}
		char const * what() const noexcept override { return msg.c_str(); }
	};

	struct File {
		FILE * const self;
		char * buffer;
		size_t length;

		File(Path const & path, string const & mode);
		File();
		~File();

		operator FILE * ();
		void writeln(string const & line);
		vector<string> readlines();
	};

	extern queue<string> argv;
	extern int indent;

	extern Path root;
	extern Path bins;
	extern Path conf;
	extern Path temp;
	extern Path pkgs;

	bool fork();
	int wait();
	int pipe(string const & cmd);
	void chown(Path const & path, unsigned int uid);

	void download(string const & url, File & file, bool display = true);
	void extract(File & archive, Path const & dir);

	void set_root(Path const & path);
	void permit();

	void stage_beg(string const & msg);
	void stage_end();

	void help();
	void list();
	void pull();
	void drop();

}