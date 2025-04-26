#include "core.h"
#include <curl/curl.h>
#include <cmath>

using namespace jpw;

static CURL * curl = nullptr;
static CURLcode code = CURLE_OK;

static str bytef(curl_off_t bytes) {
	if (bytes == 0) return "   0B";
	auto i = int(floor(log(0.0 + bytes) / log(1024.0)));
	auto u = i == 0 ? 'B' : i == 1 ? 'K' : i == 2 ? 'M' : 'G';
	auto s = bytes / pow(1024.0, i);
	return f(s < 10 ? "%1.2f%c" : s < 100 ? "%2.1f%c" : "%4.0f%c", s, u);
}

static size_t write_callback(char *ptr, size_t size, size_t nmemb, IO * io) { 
	return io->write(ptr, size * nmemb);
}

static int progress_callback(char * label, curl_off_t total, curl_off_t now, curl_off_t, curl_off_t) {
	log("\033[A\r\033[0K", "\r");
	int lw = total > 0 ? std::max(28 - indent, 0) : std::max(74 - indent, 0);
	auto tf = bytef(total);
	auto nf = bytef(now);

	if (total > 0)
		log(f("%-*s %3.0f%% [%-32.*s] %s %s", lw, label, 100.0 * now / total, (int) (32.0 * now / total), "################################", nf.c_str(), tf.c_str()));
	else
		log(f("%-*s %s", lw, label, nf.c_str()));
	return 0;
}

bool jpw::urldump(IO & io, str const & url, bool display) {
	if (!curl) {
		require(curl = curl_easy_init());
		require(curl_easy_setopt(curl, CURLOPT_FAILONERROR, true) == CURLE_OK);
		require(curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, true) == CURLE_OK);
		require(curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, progress_callback) == CURLE_OK);
		require(curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback) == CURLE_OK);
	}

	str label = url.rfind('/') != str::npos ? url.substr(url.rfind('/') + 1) : url;
	require(curl_easy_setopt(curl, CURLOPT_URL, url.c_str()) == CURLE_OK);
	require(curl_easy_setopt(curl, CURLOPT_WRITEDATA, &io) == CURLE_OK);
	require(curl_easy_setopt(curl, CURLOPT_NOPROGRESS, !display) == CURLE_OK);
	require(curl_easy_setopt(curl, CURLOPT_XFERINFODATA, label.c_str()) == CURLE_OK);

	if (display)
		log(label);

	if ((code = curl_easy_perform(curl)) != CURLE_OK) {
		static str lasturl = url;
		static size_t tries = 0;

		if (lasturl != url) {
			lasturl = url;
			tries = 0;
		}
		
		if (display) 
			log("\033[A\r\033[0K", "\r");

		return ++tries <= 3 && urldump(io, url, display);
	}

	io.flush();
	return true;
}
