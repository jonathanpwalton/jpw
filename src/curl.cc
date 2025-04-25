#include "core.h"
#include <curl/curl.h>

using namespace jpw;

static CURL * curl = nullptr;
static CURLcode code = CURLE_OK;

static int progress_callback(char * label, curl_off_t total, curl_off_t now, curl_off_t, curl_off_t) {
	print(f("\033[A\r\033[0K"), jpw::stdout, "\r");
	print(f("%s: %ld/%ld", label, now, total));
	return 0;
}

bool jpw::urlopen(IO & io, str const & url, bool display) {
	if (!curl) {
		require(curl = curl_easy_init());
		require(curl_easy_setopt(curl, CURLOPT_FAILONERROR, true) == CURLE_OK);
		require(curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, true) == CURLE_OK);
		require(curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, progress_callback) == CURLE_OK);
	}

	str label = url.rfind('/') != str::npos ? url.substr(url.rfind('/') + 1) : url;

	require(curl_easy_setopt(curl, CURLOPT_URL, url.c_str()) == CURLE_OK);
	require(curl_easy_setopt(curl, CURLOPT_WRITEDATA, io.file) == CURLE_OK);
	require(curl_easy_setopt(curl, CURLOPT_NOPROGRESS, !display) == CURLE_OK);
	require(curl_easy_setopt(curl, CURLOPT_XFERINFODATA, label.c_str()) == CURLE_OK);

	if (display) print();
	if ((code = curl_easy_perform(curl)) != CURLE_OK) {
		static str lasturl = url;
		static size_t tries = 0;

		if (lasturl != url) {
			lasturl = url;
			tries = 0;
		}
		
		if (display) print(f("\033[A\r\033[0K"), jpw::stdout, "\r");
		return ++tries <= 3 && urlopen(io, url, display);
	}

	io.flush();
	return true;
}
