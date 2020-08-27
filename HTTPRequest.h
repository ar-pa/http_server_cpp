//
// Created by arpa on 8/25/20.
//

#include <map>
#include <regex>
#include <iostream>
#include <fstream>
#include <utility>

#ifndef HTTPSERVER_HTTPREQUEST_H
#define HTTPSERVER_HTTPREQUEST_H

#endif //HTTPSERVER_HTTPREQUEST_H

class MultipartFormFile {
    std::string filename;
    std::string content_type;
public:
    MultipartFormFile(std::string filename, std::string contentType) : filename(std::move(filename)),
								       content_type(std::move(contentType)) {}

    [[nodiscard]] const std::string &get_filename() const {
	    return filename;
    }

    [[nodiscard]] const std::string &get_content_type() const {
	    return content_type;
    }
};

class MultipartFormData {
    std::string name;
    std::string content;
    std::optional<MultipartFormFile> file_description;
public:
    [[nodiscard]] const std::string &get_name() const {
	    return name;
    }

    void set_name(const std::string &name) {
	    MultipartFormData::name = name;
    }

    [[nodiscard]] const std::string &get_content() const {
	    return content;
    }

    void set_content(const std::string &content) {
	    MultipartFormData::content = content;
    }

    [[nodiscard]] const std::optional<MultipartFormFile> &get_file_description() const {
	    return file_description;
    }

    void set_file_description(const MultipartFormFile &fileDescription) {
	    file_description = fileDescription;
    }
};

class HTTPRequest {
    std::string method, target, path, body, remote_ip;
    std::map<std::string, std::string> headers;
    std::string http_version;
    std::map<std::string, std::string> params;
    std::optional<std::vector<MultipartFormData>> data_parts;
    bool bad_request;
private:
    static std::vector<std::string> split(const std::string &s, const std::string &delimiter) {
	    std::vector<std::string> res;
	    std::string cur;
	    for (int i = 0; i < s.size(); i++)
		    if (i + delimiter.size() <= s.size() && s.substr(i, delimiter.size()) == delimiter) {
			    if (!cur.empty())
				    res.push_back(cur);
			    cur = "";
			    i += delimiter.size() - 1;
		    } else
			    cur += s[i];
	    if (!cur.empty())
		    res.push_back(cur);
	    return res;
    }

    bool parse_request_line(const std::string &request_line) {
	    const static std::regex regex(
		    "(GET|HEAD|POST) "
		    "(([^?]+)(?:\\?(.*?))?) (HTTP/1\\.[01])");

	    std::cmatch m;
	    if (std::regex_match(request_line.c_str(), m, regex)) {
		    method = m[1];
		    target = m[2];
		    path = m[3];
		    for (const auto &parameter : split(m[4], "&"))
			    params[split(parameter, "=")[0]] = split(parameter, "=")[1];
		    return true;
	    }
	    return false;
    }

    bool parse_multipart(std::string boundary) {
	    data_parts = std::vector<MultipartFormData>();
	    while (!body.empty() && body.back() != boundary.back()) {
		    body.pop_back();
	    }
	    auto parts = split(body, boundary + "\r\n");
	    for (const auto &part : parts) {
		    MultipartFormData data;
		    try {
			    auto get_field = [&part](const std::string &field) {
				int p = part.find('\"', part.find(field)) + 1;
				return part.substr(p, part.find('\"', p) - p);
			    };
			    data.set_name(get_field("name"));
			    {
				    int p = part.find("\r\n\r\n");
				    data.set_content(part.substr(p));
			    }
			    if (part.find("filename") != std::string::npos && part.find("filename") < part.find('\n')) {
				    int start_second = part.find("Content-Type");
				    std::string con = part.substr(start_second + 14,
								  part.find('\r', start_second) - (start_second + 14));
				    data.set_file_description(MultipartFormFile(get_field("filename"), con));
			    }
			    data_parts.value().push_back(data);
		    }
		    catch (std::exception &e) {
			    std::cerr << "\n==part==\n" << part << "\n==end part==\n";
			    std::cerr << e.what() << '\n';
			    return false;
		    }
		    catch (...) {
			    return false;
		    }
	    }
	    return true;
    }

public:
    [[nodiscard]] const std::string &get_method() const {
	    return method;
    }

    [[nodiscard]] const std::string &get_target() const {
	    return target;
    }

    [[nodiscard]] const std::string &get_path() const {
	    return path;
    }

    [[nodiscard]] const std::string &get_body() const {
	    return body;
    }

    [[nodiscard]] const std::string &get_remote_ip() const {
	    return remote_ip;
    }

    [[nodiscard]] const std::string &get_http_version() const {
	    return http_version;
    }

    std::optional<std::string> get_header(const std::string &header) {
	    if (headers.count(header))
		    return headers[header];
	    return {};
    }

    std::vector<std::pair<std::string, std::string>> get_headers() {
	    return std::vector<std::pair<std::string, std::string>>(headers.begin(), headers.end());
    }

    [[nodiscard]] bool is_bad() const {
	    return bad_request;
    }

    [[nodiscard]] const std::optional<std::vector<MultipartFormData>> &get_data_parts() const {
	    return data_parts;
    }

    void set_header(const std::string &header, const std::string &value) {
	    headers[header] = value;
    }

    explicit HTTPRequest(const std::string &plain_request) {
    	std::ofstream ofstream("tmp");
    	ofstream << plain_request;
    	ofstream.close();

	    try {
		    int body_start = plain_request.find("\r\n\r\n");
		    body = plain_request.substr(body_start + 4);
		    auto head = split(plain_request.substr(0, body_start), "\r\n");
		    if (!parse_request_line(head[0])) {
			    bad_request = true;
			    return;
		    }
		    for (int i = 1; i < head.size(); i++)
			    headers[split(head[i], ": ")[0]] = split(head[i], ": ")[1];
		    if (get_header("Content-Type").has_value() &&
			get_header("Content-Type").value().find("multipart/form-data") == 0) {
			    std::string tmp = get_header("Content-Type").value();
			    int p = tmp.find("--");
			    if (!parse_multipart("--" + tmp.substr(p)))
				    bad_request = true;
		    }
	    }
	    catch (std::exception& e) {
		bad_request = true;
	    }
    }
};