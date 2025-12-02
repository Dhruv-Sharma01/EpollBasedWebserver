#pragma once

#include <string>
#include <string_view>
#include <sstream>
#include <unordered_map>
#include <span>
#include <iostream>

struct HttpRequest {
    std::string method;
    std::string path;
    std::string version;
    std::unordered_map<std::string, std::string> headers;
    std::string body;
};

struct HttpResponse {
    int statusCode = 200;
    std::string statusMessage = "OK";
    std::unordered_map<std::string, std::string> headers;
    std::string body;

    std::string toString() const {
        std::ostringstream oss;
        oss << "HTTP/1.1 " << statusCode << " " << statusMessage << "\r\n";
        for (const auto& [key, val] : headers) {
            oss << key << ": " << val << "\r\n";
        }
        oss << "Content-Length: " << body.size() << "\r\n";
        oss << "Connection: close\r\n"; // For simplicity in this implementation
        oss << "\r\n";
        oss << body;
        return oss.str();
    }
};

class HttpUtils {
public:
    static HttpRequest parseRequest(std::span<const char> data) {
        HttpRequest req;
        std::string_view raw(data.data(), data.size());

        // Find end of headers
        size_t headerEnd = raw.find("\r\n\r\n");
        if (headerEnd == std::string_view::npos) {
            return req; // Incomplete or invalid
        }

        std::string_view headerPart = raw.substr(0, headerEnd);
        std::string_view bodyPart = raw.substr(headerEnd + 4);
        req.body = std::string(bodyPart);

        std::istringstream stream{std::string(headerPart)};
        std::string line;

        // Request Line
        if (std::getline(stream, line)) {
            if (!line.empty() && line.back() == '\r') line.pop_back();
            std::istringstream lineStream(line);
            lineStream >> req.method >> req.path >> req.version;
        }

        // Headers
        while (std::getline(stream, line)) {
            if (!line.empty() && line.back() == '\r') line.pop_back();
            if (line.empty()) break;

            size_t colon = line.find(':');
            if (colon != std::string::npos) {
                std::string key = line.substr(0, colon);
                std::string val = line.substr(colon + 1);
                // Trim whitespace
                size_t first = val.find_first_not_of(' ');
                if (std::string::npos != first) val = val.substr(first);
                req.headers[key] = val;
            }
        }

        return req;
    }
};
