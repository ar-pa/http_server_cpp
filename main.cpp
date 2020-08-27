#include <iostream>
#include <sys/stat.h>
#include <csignal>
#include "HTTPServer.h"
#include "Interrupt_handler.h"

std::string read_file(const std::string &path);

static std::string find_content_type(const std::string &ext);

inline bool is_file(const std::string &path);

std::string file_extension(const std::string &path);

int main(int argc, char *argv[]) {
	int port = argc > 1 ? std::stoi(argv[1]) : 8080;
	signal(SIGINT, &Interrupt_handler::handle);
	auto *http = new HTTPServer([](const HTTPRequest &request) {
//	    std::cerr << "== multipart ==\n\n";
	    if (request.is_bad())
		    return HTTPResponse(400, "Bad request.");
	    if (request.get_method() == "GET" || request.get_method() == "HEAD") {
		    std::string path = request.get_path();
		    if (path.back() == '/') {
			    path += "index.html";
		    }
//		    std::cerr << path << '\n';
		    path = path.substr(1);
		    if (is_file(path)) {
			    HTTPResponse response(200);
//			    std::cerr << path << '\n';
			    response.set_body(read_file(path));
			    response.set_header("Content-Type", find_content_type(file_extension(path)));
			    return response;
		    } else {
			    return HTTPResponse(404, "File not found.\n");
		    }
	    }
	    if (request.get_data_parts()) {
		    std::string form_names;
		    for (const auto &part : request.get_data_parts().value()) {
			    form_names += part.get_name();
			    if (part.get_file_description())
				    form_names += ": " + part.get_file_description().value().get_filename();
			    form_names += '\n';
		    }
		    return HTTPResponse(200, "Hello.\nYou have submitted a form with contents:\n" + form_names);
	    }
	    return HTTPResponse(200, "Hello.\n");
	});
	Interrupt_handler::watch(http);
	http->listen(port);
}

std::string read_file(const std::string &path) {
	std::ifstream ifstream(path, std::ios_base::binary);
	std::stringstream ss;
	ss << ifstream.rdbuf();
	return ss.str();
}

bool is_file(const std::string &path) {
	struct stat st{};
	return stat(path.c_str(), &st) >= 0 && S_ISREG(st.st_mode);
}

std::string find_content_type(const std::string &ext) {
	if (ext == "txt") {
		return "text/plain";
	} else if (ext == "html" || ext == "htm") {
		return "text/html";
	} else if (ext == "css") {
		return "text/css";
	} else if (ext == "jpeg" || ext == "jpg") {
		return "image/jpg";
	} else if (ext == "png") {
		return "image/png";
	} else if (ext == "gif") {
		return "image/gif";
	} else if (ext == "svg") {
		return "image/svg+xml";
	} else if (ext == "ico") {
		return "image/x-icon";
	} else if (ext == "json") {
		return "application/json";
	} else if (ext == "pdf") {
		return "application/pdf";
	} else if (ext == "js") {
		return "application/javascript";
	} else if (ext == "wasm") {
		return "application/wasm";
	} else if (ext == "xml") {
		return "application/xml";
	} else if (ext == "xhtml") {
		return "application/xhtml+xml";
	}
	return "";
}

std::string file_extension(const std::string &path) {
	std::smatch m;
	static auto re = std::regex("\\.([a-zA-Z0-9]+)$");
	if (std::regex_search(path, m, re)) { return m[1].str(); }
	return std::string();
}
