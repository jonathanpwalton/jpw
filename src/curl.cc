#include "jpw.h"

#include <curl/curl.h>
#include <math.h>

static CURL * curl = nullptr;

static jpw::string bytes(size_t bytes) {
	if (bytes == 0) return "   0B";
	auto i = (size_t) floor(log((double) bytes) / log((double) 1024));
	auto u = i == 0 ? 'B' : i == 1 ? 'K' : i == 2 ? 'M' : 'G';
	auto b = bytes / pow(1024.0, i);
	char s[5];
	sprintf(s, b >= 100 ? "%4.0f%c" : b >= 10 ? "%2.1f%c" : "%1.2f%c", b, u);
	return s;
}

static int progress(char const * label, curl_off_t total, curl_off_t now, curl_off_t, curl_off_t) {
	using namespace jpw;

	printf("\033[A\r\033[0K");
	if (total == 0) printf("%*s%-74.74s %s\n", indent, "", label, bytes(now).c_str());
	else printf("%*s%-28.28s %3d%% [%-32.*s] %s %s\n", indent, "", label, (int) ((double) now / total * 100.0), (int) ((double) now / total * 32.0), "################################", bytes(now).c_str(), bytes(total).c_str());
	return 0;
}

void jpw::download(string const & url, File & file, bool display) {
	if (!curl) {
		if (!(curl = curl_easy_init()))
			throw std::runtime_error("failed to initialize curl");

		curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, true);
		curl_easy_setopt(curl, CURLOPT_FAILONERROR, true);
		curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, progress);
	}

	curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
	curl_easy_setopt(curl, CURLOPT_NOPROGRESS, !display);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, file.self);
	curl_easy_setopt(curl, CURLOPT_XFERINFODATA, url.c_str() + (url.rfind('/') != string::npos ? url.rfind('/') + 1 : 0));

	if (display)
		printf("%*srequesting %s\n", indent, "", url.c_str());

	auto code = curl_easy_perform(curl);
	fflush(file);

	if (code != CURLE_OK) {
		if (display)
			printf("\033[A\r\033[0K");

		static string last_url = url;
		static int tries = 0;

		if (last_url == url && tries >= 3)
			throw ProgramError("failed to retrieve " + url + " (" + curl_easy_strerror(code) + ")");

		if (last_url != url) {
			last_url = url;
			tries = 0;
		}

		tries++;
		download(url, file, display);
	}

	fflush(file);
}