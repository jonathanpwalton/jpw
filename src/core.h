#pragma once

#include <cstdio>
#include <string>
#include <vector>
#include <cstdint>
#include <cstdarg>
#include <filesystem>

#ifdef JPW_IMPLEMENTATION
#define JPW_EXTERN
#define JPW_EXTERN_VALUE(X)
#else
#define JPW_EXTERN extern
#define JPW_EXTERN_VALUE(X) = X
#endif

#undef stdout
#undef stderr
#undef stdin

namespace jpw {

	using str = std::string;
	using Path = std::filesystem::path;
	namespace fs { using namespace std::filesystem; }

	template<typename T>
	class list: public std::vector<T> {
	public:
		using std::vector<T>::vector;

		constexpr auto & append(const T & value) {
			this->push_back(value);
			return this->back();
		}

		constexpr auto & append(T && value) {
			this->push_back(value);
			return this->back();
		}

		constexpr auto pop(intmax_t index = -1) {
			if (index == -1) {
				auto val = this->back();
				this->pop_back();
				return val;
			}
			
			if (index < 0) index = this->size() + index;
			auto val = this->at(index);
			this->erase(this->begin() + index);
			return val;
		}

		constexpr auto slice(intmax_t start = 0, intmax_t stop = INTMAX_MAX, intmax_t step = 1) {
			list<T> r;

			if (start < 0) start += this->size();
			if (stop < 0)	stop += this->size();
			else if (stop == INTMAX_MAX) stop = this->size();
			if (start < 0 || (size_t) start >= this->size()) return r;
			if (stop < 0 || (size_t) stop > this->size()) return r;

			for (intmax_t i = start; i < stop; i += step)
				r += this->at(i);

			return r;
		}
	};

	inline str operator * (const str & s, size_t n) {
		str result; for (size_t i = 0; i < n; i++) result += s;
		return result;
	}

	inline auto len(str const & s) {
		return s.length();
	}

	template<typename T>
	inline constexpr auto len(list<T> const & list) {
		return list.size();
	}

	inline str f(str const & s, ...) {
		static char r[4096];
		va_list ap;
		va_start(ap, s);
		vsnprintf(r, 4096, s.c_str(), ap);
		va_end(ap);
		return r;
	}

	inline void print(str const & s = "", FILE * file = stdout, str const & end = "\n") {
		fprintf(file, "%s%s", s.c_str(), end.c_str());
	}

	void set_root(Path root);
	void require_permission();

	int main_help();
	int main_pull();

	JPW_EXTERN str program;
	JPW_EXTERN list<str> argv;
	JPW_EXTERN int indent;

	JPW_EXTERN Path root_path;
	JPW_EXTERN Path etc_path;
	JPW_EXTERN Path lib_path;

	template<typename T>
	struct Maybe {
		bool has_value = false;
		T value;
		Maybe() : has_value(false) {}
		Maybe(bool has) : has_value(has) {}
		Maybe(T const & t) : has_value(true), value(t) {}
		Maybe(T && t) : has_value(true), value(t) {}
		operator bool () { return has_value; }
		operator T & () { return value; }
	};

	struct IO {
		FILE * const file = nullptr;
		IO(FILE * file) : file(file) {}
		operator FILE * () { return file; }

		virtual list<uint8_t> read() { return {}; }

		virtual void flush() { fflush(file); }
		virtual void write(char c) { fputc(c, file); }
		virtual size_t write(void const * data, size_t bytes ) { return fwrite(data, 1, bytes, file); }
		virtual void write(str const & s, str const & end = "\n") { fprintf(file, s.c_str()); if (!end.empty()) fprintf(file, end.c_str()); }
		virtual void write(list<str> const & lines) { for (auto & line : lines) write(line); }
		virtual Maybe<str> readline();
		list<str> readlines();
	};
	
	static IO stdin(::stdin);
	static IO stdout(::stdout);
	static IO stderr(::stderr);
	
	struct File : public IO {
		File(Path const & path, str const & mode = "r") : IO(fopen(path.c_str(), mode.c_str())) {}
		~File() { fclose(file); }

		virtual list<uint8_t> read() override {
			list<uint8_t> bytes;
			fseek(file, 0, SEEK_END);
			bytes.resize(ftell(file));
			fseek(file, 0, SEEK_SET);
			fread(bytes.data(), bytes.size(), 1, file);
			return bytes;
		}
	};

	struct BytesIO : public IO {
		list<uint8_t> buffer;
		size_t offset = 0;

		BytesIO() : IO(nullptr) {}

		virtual list<uint8_t> read() override { return buffer; }
		virtual void flush() {}
		virtual void write(char c) override { buffer.append((uint8_t) c); }
		virtual size_t write(void const * data, size_t bytes ) { size_t s = len(buffer); buffer.insert(buffer.end(), (uint8_t *) data, (uint8_t *) data + bytes * sizeof(uint8_t)); return len(buffer) - s; }
		virtual void write(str const & s, str const & end = "\n") override { str q = s + end; buffer.insert(buffer.end(), q.begin(), q.end()); }
		virtual Maybe<str> readline() override;
	};

	bool urldump(IO & io, str const & url, bool display = true);

	inline void stage_beg(str const & label) { print(f("\033[0m\033[1;97m%*s%s %s\033[0m", indent, "", indent == 0 ? "::" : "=>", label.c_str())); indent += 3; }
	inline void stage_end() { indent -= 3; }

	inline void error(str const & s = "") { print(f("%*s\033[1;91merror: \033[0m%s", indent, "", s.c_str()), jpw::stderr); exit(1); }
	inline void log(str const & s, str const & end = "\n") { print(f("%*s%s", indent, "", s.c_str()), jpw::stdout, end); }

	bool extract_archive(IO & src, Path dst);

}

#define require(Condition) if (!(Condition)) { jpw::error(jpw::f("%s:%d: %s: assertion `" #Condition "` failed", __FILE__, __LINE__, __func__)); }
#define TODO() jpw::error(jpw::f("%s:%d: %s: TODO", __FILE__, __LINE__, __func__))
