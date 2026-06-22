# include "../includes/client.hpp"
# include <errno.h>
# include <cctype>
# include <sstream>

static std::string trimCopy(const std::string& s)
{
	size_t begin = 0;
	size_t end = s.size();
	while (begin < end && (s[begin] == ' ' || s[begin] == '\t' || s[begin] == '\r' || s[begin] == '\n'))
		++begin;
	while (end > begin && (s[end - 1] == ' ' || s[end - 1] == '\t' || s[end - 1] == '\r' || s[end - 1] == '\n'))
		--end;
	return s.substr(begin, end - begin);
}

static std::string basenameCopy(const std::string& path)
{
	size_t slash = path.find_last_of('/');
	std::string name = (slash == std::string::npos) ? path : path.substr(slash + 1);
	if (name.empty())
		return "";
	return name;
}

static std::string sanitizeFilename(const std::string& name)
{
	std::string base = basenameCopy(trimCopy(name));
	if (base.empty() || base == "." || base == "..")
		return "";
	std::string result;
	for (size_t i = 0; i < base.size(); ++i)
	{
		unsigned char c = static_cast<unsigned char>(base[i]);
		if (std::isalnum(c) || c == '.' || c == '-' || c == '_')
			result += static_cast<char>(c);
		else
			result += '_';
	}
	if (result.empty() || result == "." || result == "..")
		return "";
	return result;
}

static std::string joinPath(const std::string& left, const std::string& right)
{
	if (left.empty())
		return right;
	if (right.empty())
		return left;
	if (left[left.size() - 1] == '/' && right[0] == '/')
		return left + right.substr(1);
	if (left[left.size() - 1] != '/' && right[0] != '/')
		return left + "/" + right;
	return left + right;
}

static std::string addSuffixToFilename(const std::string& filename, size_t suffix)
{
	if (suffix == 0)
		return filename;
	size_t dot = filename.find_last_of('.');
	std::ostringstream oss;
	if (dot == std::string::npos || dot == 0)
		oss << filename << '_' << suffix;
	else
		oss << filename.substr(0, dot) << '_' << suffix << filename.substr(dot);
	return oss.str();
}

static bool parseBoundaryParameter(const std::string& contentType, std::string& boundary)
{
	std::string lower = contentType;
	for (size_t i = 0; i < lower.size(); ++i)
		lower[i] = static_cast<char>(std::tolower(static_cast<unsigned char>(lower[i])));
	size_t pos = lower.find("boundary=");
	if (pos == std::string::npos)
		return false;
	pos += 9;
	while (pos < contentType.size() && (contentType[pos] == ' ' || contentType[pos] == '\t'))
		++pos;
	if (pos >= contentType.size())
		return false;
	if (contentType[pos] == '"')
	{
		size_t end = contentType.find('"', pos + 1);
		if (end == std::string::npos)
			return false;
		boundary = contentType.substr(pos + 1, end - pos - 1);
	}
	else
	{
		size_t end = contentType.find(';', pos);
		boundary = contentType.substr(pos, end == std::string::npos ? std::string::npos : end - pos);
	}
	boundary = trimCopy(boundary);
	return !boundary.empty();
}

static std::string extractMultipartFilename(const std::string& headers)
{
	std::istringstream stream(headers);
	std::string line;
	while (std::getline(stream, line))
	{
		line = trimCopy(line);
		if (line.empty())
			continue;
		std::string lower = line;
		for (size_t i = 0; i < lower.size(); ++i)
			lower[i] = static_cast<char>(std::tolower(static_cast<unsigned char>(lower[i])));
		if (lower.find("content-disposition") != std::string::npos)
		{
			size_t pos = lower.find("filename=");
			if (pos == std::string::npos)
				continue;
			pos += 9;
			while (pos < line.size() && (line[pos] == ' ' || line[pos] == '\t'))
				++pos;
			if (pos >= line.size())
				continue;
			if (line[pos] == '"')
			{
				size_t end = line.find('"', pos + 1);
				if (end == std::string::npos)
					continue;
				return sanitizeFilename(line.substr(pos + 1, end - pos - 1));
			}
			else
			{
				size_t end = line.find(';', pos);
				return sanitizeFilename(line.substr(pos, end == std::string::npos ? std::string::npos : end - pos));
			}
		}
	}
	return "";
}

// Initialize request/response state for a new client.
client::client()
	: is_finished_reading(false), bytes_sent(0), file_fd(-1), file_size(0), file_offset(0),
	  streaming_file(false), upload_fd(-1), uploaded_bytes(0), max_upload_size(0),
	  is_uploading(false), upload_completed(false), upload_error_code(0), upload_dir(""), upload_path(""),
	  upload_url(""), upload_pending(""), upload_pending_offset(0),
	  upload_is_multipart(false), upload_boundary(""), upload_buffer(""),
 	  upload_headers_parsed(false),
	  cgi(NULL), cgi_in(""), cgi_in_off(0), cgi_out(""), cgi_server(NULL),
	  cgi_start(0), cgi_active(false) { last_activity = std::time(NULL); }

// Add activity timestamp initialization
void client::touch_activity()
{
    last_activity = std::time(NULL);
}

time_t client::get_last_activity() const
{
    return last_activity;
}

client&	client::operator = (const client& src)
{
	if (this == &src)
		return (*this);

	this->fd = src.fd;
	this->request = src.request;
	this->bytes_sent = src.bytes_sent;
	this->is_finished_reading = src.is_finished_reading;
	this->response_data = src.response_data;
	// Do not duplicate file descriptors when copying; mark not streaming.
	this->file_fd = -1;
	this->file_size = 0;
	this->file_offset = 0;
	this->streaming_file = false;

	// Reset all upload bookkeeping to safe defaults. A client is only ever
	// copied at insertion time (before any upload/CGI is owned), so we must
	// never inherit an fd or a half-initialized state from the source — doing
	// so previously left upload_fd indeterminate and closed a random fd in the
	// copy's destructor.
	this->upload_fd = -1;
	this->uploaded_bytes = 0;
	this->max_upload_size = 0;
	this->is_uploading = false;
	this->upload_completed = false;
	this->upload_error_code = 0;
	this->upload_dir.clear();
	this->upload_path.clear();
	this->upload_url.clear();
	this->upload_pending.clear();
	this->upload_pending_offset = 0;
	this->upload_is_multipart = false;
	this->upload_boundary.clear();
	this->upload_buffer.clear();
	this->upload_headers_parsed = false;

	// Never copy CGI ownership (raw pointer / child process).
	this->cgi = NULL;
	this->cgi_in.clear();
	this->cgi_in_off = 0;
	this->cgi_out.clear();
	this->cgi_server = NULL;
	this->cgi_start = 0;
	this->cgi_active = false;

	// Preserve last activity timestamp when copying client state
	this->last_activity = src.last_activity;

	return (*this);
}


client::client(const client& src)
{
	(*this) = src ;
}

client::~client()
{
	clear_cgi();
	reset_upload_state();
}

// Assign fd and reset per-connection response state.
void	client::set_client_fd(int clien_fd)
{
	fd = clien_fd;
	bytes_sent = 0;
	response_data.clear();
	if (file_fd >= 0)
	{
		close(file_fd);
		file_fd = -1;
	}
	file_size = 0;
	file_offset = 0;
	streaming_file = false;
	reset_upload_state();
	touch_activity();
}


void	client::set_finished_reading(bool var) {  is_finished_reading = var; }

bool	client::get_finished_reading() { return (is_finished_reading); }

int	client::get_client_id(){ return(fd); }


void	client::parse_request(char *data, int rb)
{
	touch_activity();
	request.parse(data, rb);
}

bool	client::is_parsing_finished()
{
	return (request.isFinished() || request.hasError());
}

// Mutable request accessor for parser integration.
Request& client::get_request()
{
	return request;
}

// Const request accessor for routing/response generation.
const Request& client::get_request() const
{
	return request;
}

// Store serialized response for later partial sends.
void client::set_response(const std::string& data)
{
	response_data = data;
}

void client::set_response_from_response(const Response& res)
{
	// Headers (and possibly body) are serialized by Response::toString()
	response_data = res.toString();
	bytes_sent = 0;

	// If Response indicates a file, open it for streaming
	if (!res.getFilePath().empty())
	{
		if (file_fd >= 0)
			close(file_fd);
		file_fd = open(res.getFilePath().c_str(), O_RDONLY);
		if (file_fd < 0)
		{
			std::cerr << "Failed to open file for streaming: " << res.getFilePath() << ": " << strerror(errno) << std::endl;
			// couldn't open file; clear streaming state. The headers already indicate 200, but
			// failure will be detected during send and result in a 500 on next attempt.
			streaming_file = false;
			file_size = 0;
			file_offset = 0;
			return;
		}
		file_size = res.getFileSize();
		file_offset = res.getFileOffset();
		streaming_file = true;
	}
	else
	{
		// No file streaming needed
		if (file_fd >= 0)
		{
			close(file_fd);
			file_fd = -1;
		}
		file_size = 0;
		file_offset = 0;
		streaming_file = false;
	}
}

// Access response payload for send().
const std::string& client::get_response() const
{
	return response_data;
}

// Track how many bytes were already written to the socket.
size_t client::get_bytes_sent() const
{
	return bytes_sent;
}

// Advance the send cursor after a successful write.
void client::add_bytes_sent(size_t n)
{
	bytes_sent += n;
}

// Reset send cursor when preparing a new response.
void client::reset_bytes_sent()
{
	bytes_sent = 0;
}

bool client::is_streaming_file() const
{
	return streaming_file;
}

int client::get_file_fd() const
{
	return file_fd;
}

off_t client::get_file_offset() const
{
	return file_offset;
}

void client::set_file_offset(off_t off)
{
	file_offset = off;
}

size_t client::get_file_size() const
{
	return file_size;
}

void client::reset_upload_state()
{
	if (upload_error_code != 0 && !upload_path.empty())
		unlink(upload_path.c_str());
	if (!upload_completed && !upload_path.empty() && upload_error_code == 0)
		unlink(upload_path.c_str());
	if (upload_fd >= 0)
	{
		close(upload_fd);
		upload_fd = -1;
	}
	uploaded_bytes = 0;
	max_upload_size = 0;
	is_uploading = false;
	upload_completed = false;
	upload_error_code = 0;
	upload_dir.clear();
	upload_path.clear();
	upload_url.clear();
	upload_pending.clear();
	upload_pending_offset = 0;
	upload_is_multipart = false;
	upload_boundary.clear();
	upload_buffer.clear();
	upload_headers_parsed = false;
}

static int open_unique_upload_file(const std::string& dir, const std::string& baseName, std::string& outPath)
{
	std::string name = baseName.empty() ? "upload.bin" : baseName;
	for (size_t suffix = 0; suffix < 1000; ++suffix)
	{
		std::string candidate = joinPath(dir, addSuffixToFilename(name, suffix));
		int fd = open(candidate.c_str(), O_WRONLY | O_CREAT | O_EXCL, 0644);
		if (fd >= 0)
		{
			outPath = candidate;
			return fd;
		}
		if (errno != EEXIST)
			return -1;
	}
	return -1;
}

bool client::begin_upload(const std::string& uploadDir, const std::string& request_path,
		const std::string& content_type, size_t max_size)
{
	reset_upload_state();
	is_uploading = true;
	max_upload_size = max_size;
	uploaded_bytes = 0;
	upload_error_code = 0;
	upload_completed = false;
	upload_is_multipart = false;
	upload_headers_parsed = false;
	upload_dir = uploadDir;

	std::string baseUrl = request_path.empty() ? "/" : request_path;
	if (baseUrl[baseUrl.size() - 1] == '/')
		baseUrl.erase(baseUrl.size() - 1);
	if (baseUrl.empty())
		baseUrl = "/";

	std::string lower = content_type;
	for (size_t i = 0; i < lower.size(); ++i)
		lower[i] = static_cast<char>(std::tolower(static_cast<unsigned char>(lower[i])));
	if (lower.find("multipart/form-data") != std::string::npos)
	{
		std::string boundary;
		if (!parseBoundaryParameter(content_type, boundary))
		{
			upload_error_code = 400;
			is_uploading = false;
			return false;
		}
		upload_is_multipart = true;
		upload_boundary = "--" + boundary;
		upload_url = baseUrl; // will be completed once filename is known
		return true;
	}

	std::string filename = sanitizeFilename(basenameCopy(request_path));
	if (filename.empty())
		filename = "upload.bin";
	upload_url = (baseUrl == "/" ? "/" + filename : baseUrl + "/" + filename);
	int fd = open_unique_upload_file(uploadDir, filename, upload_path);
	if (fd < 0)
	{
		upload_error_code = 500;
		is_uploading = false;
		return false;
	}
	upload_fd = fd;
	return true;
}

bool client::flush_upload_pending()
{
	if (upload_pending.empty())
		return true;
	if (upload_fd < 0)
	{
		upload_error_code = 500;
		return false;
	}
	const char* data = upload_pending.data() + upload_pending_offset;
	size_t left = upload_pending.size() - upload_pending_offset;
	ssize_t w = write(upload_fd, data, left);
	if (w < 0)
	{
		upload_error_code = 500;
		return false;
	}
	upload_pending_offset += static_cast<size_t>(w);
	if (upload_pending_offset >= upload_pending.size())
	{
		upload_pending.clear();
		upload_pending_offset = 0;
		return true;
	}
	return false;
}

bool client::upload_in_progress() const
{
	return is_uploading;
}

bool client::upload_done() const
{
	return upload_completed;
}

int client::get_upload_error_code() const
{
	return upload_error_code;
}

const std::string& client::get_upload_url() const
{
	return upload_url;
}

int client::write_or_buffer(const char* data, size_t len)
{
	if (max_upload_size > 0 && uploaded_bytes + len > max_upload_size)
		return -413;
	if (upload_fd < 0)
		return -500;
	ssize_t w = write(upload_fd, data, len);
	if (w == static_cast<ssize_t>(len))
	{
		uploaded_bytes += len;
		return BodySink::BODY_OK;
	}
	if (w >= 0)
	{
		upload_pending.assign(data + w, data + len);
		upload_pending_offset = 0;
		uploaded_bytes += len;
		return BodySink::BODY_OK;
	}
	return -500;
}

int client::onBodyData(const char* data, size_t len)
{
	if (!is_uploading)
		return BodySink::BODY_ERROR;
	if (!upload_pending.empty())
		return BodySink::BODY_AGAIN;
	if (upload_completed)
		return BodySink::BODY_OK;

	if (!upload_is_multipart)
	{
		int res = write_or_buffer(data, len);
		if (res < 0)
			upload_error_code = -res;
		if (res == BodySink::BODY_OK && !upload_pending.empty())
			return BodySink::BODY_AGAIN;
		return res;
	}

	// Multipart streaming: buffer and parse incrementally.
	upload_buffer.append(data, len);
	while (true)
	{
		if (!upload_headers_parsed)
		{
			size_t bpos = upload_buffer.find(upload_boundary);
			if (bpos == std::string::npos)
			{
				if (upload_buffer.size() > upload_boundary.size())
					upload_buffer.erase(0, upload_buffer.size() - upload_boundary.size());
				return BodySink::BODY_OK;
			}
			size_t after = bpos + upload_boundary.size();
			if (upload_buffer.size() < after + 2)
				return BodySink::BODY_OK;
			if (upload_buffer.compare(after, 2, "--") == 0)
			{
				upload_completed = true;
				return BodySink::BODY_OK;
			}
			if (upload_buffer.compare(after, 2, "\r\n") != 0)
				return -400;
			upload_buffer.erase(0, after + 2);
			size_t hdrEnd = upload_buffer.find("\r\n\r\n");
			if (hdrEnd == std::string::npos)
				return BodySink::BODY_OK;
			std::string headers = upload_buffer.substr(0, hdrEnd);
			upload_buffer.erase(0, hdrEnd + 4);
			std::string filename = extractMultipartFilename(headers);
			if (filename.empty())
				filename = "upload.bin";
			int fd = open_unique_upload_file(upload_dir, filename, upload_path);
			if (upload_fd >= 0)
				close(upload_fd);
			upload_fd = fd;
			if (upload_fd < 0)
				return -500;
			upload_url = (upload_url == "/" ? "/" + filename : upload_url + "/" + filename);
			upload_headers_parsed = true;
			continue;
		}
		else
		{
			std::string marker = "\r\n" + upload_boundary;
			size_t mpos = upload_buffer.find(marker);
			if (mpos == std::string::npos)
			{
				if (upload_buffer.size() > marker.size())
				{
					size_t writeLen = upload_buffer.size() - marker.size();
					int res = write_or_buffer(upload_buffer.data(), writeLen);
					if (res != BodySink::BODY_OK)
					{
						if (res < 0)
							upload_error_code = -res;
						return res;
					}
					if (!upload_pending.empty())
						return BodySink::BODY_AGAIN;
					upload_buffer.erase(0, writeLen);
				}
				return BodySink::BODY_OK;
			}
			if (mpos > 0)
			{
				int res = write_or_buffer(upload_buffer.data(), mpos);
				if (res != BodySink::BODY_OK)
				{
					if (res < 0)
						upload_error_code = -res;
					return res;
				}
				if (!upload_pending.empty())
					return BodySink::BODY_AGAIN;
			}
			upload_buffer.erase(0, mpos + marker.size());
			upload_completed = true;
			return BodySink::BODY_OK;
		}
	}
}

void client::onBodyEnd()
{
	if (upload_is_multipart && upload_path.empty())
	{
		upload_error_code = 400;
		request.setErrorCode(400);
	}
	if (upload_fd >= 0)
	{
		close(upload_fd);
		upload_fd = -1;
	}
	upload_completed = true;
	is_uploading = false;
	request.setUploadResult(upload_path, upload_url);
}


// ─── async CGI state ────────────────────────────────────────────────────────

void client::attach_cgi(CgiHandler* handler, const std::string& input, const ServerConfig* server)
{
	cgi = handler;
	cgi_in = input;
	cgi_in_off = 0;
	cgi_out.clear();
	cgi_server = server;
	cgi_start = std::time(NULL);
	cgi_active = (handler != NULL);
}

bool client::cgi_is_active() const { return cgi_active; }

bool client::cgi_has_input() const { return !cgi_in.empty(); }

CgiHandler* client::get_cgi() { return cgi; }

const std::string& client::get_cgi_input() const { return cgi_in; }

size_t client::get_cgi_in_off() const { return cgi_in_off; }

void client::add_cgi_in_off(size_t n) { cgi_in_off += n; }

void client::append_cgi_out(const char* data, size_t n) { cgi_out.append(data, n); }

const std::string& client::get_cgi_out() const { return cgi_out; }

const ServerConfig* client::get_cgi_server() const { return cgi_server; }

time_t client::get_cgi_start() const { return cgi_start; }

void client::clear_cgi()
{
	if (cgi)
	{
		delete cgi;   // CgiHandler dtor closes any open pipe ends and reaps the child
		cgi = NULL;
	}
	cgi_in.clear();
	cgi_in_off = 0;
	cgi_out.clear();
	cgi_server = NULL;
	cgi_start = 0;
	cgi_active = false;
}
