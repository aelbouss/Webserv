# ifndef CLIENT_HPP
# define CLIENT_HPP

# include "server_infra.hpp"
# include "request.hpp"
# include "response.hpp"
# include <string>
# include <ctime>


class	client : public BodySink
{

	private:
		int	fd;
		Request	request;
		bool	is_finished_reading;
		size_t	bytes_sent;
		// Buffered response payload for partial writes.
		std::string response_data;
		// File streaming state (used when serving files)
		int file_fd;
		size_t file_size;
		off_t file_offset;
		bool streaming_file;

		// Upload streaming state
		int upload_fd;
		size_t uploaded_bytes;
		size_t max_upload_size;
		bool is_uploading;
		bool upload_completed;
		int upload_error_code;
		std::string upload_dir;
		std::string upload_path;
		std::string upload_url;
		std::string upload_pending;
		size_t upload_pending_offset;
		bool upload_is_multipart;
		std::string upload_boundary;
		std::string upload_buffer;
		bool upload_headers_parsed;
		int	write_or_buffer(const char* data, size_t len);

		// Last activity timestamp for timeout handling
		time_t last_activity;

	public:
		void touch_activity();
		time_t get_last_activity() const;

		client();
		client&	operator = (const client& src);
		client(const client& src);
		~client();
		void	set_client_fd(int client_fd);
		int		get_client_id();
		void	set_finished_reading(bool var);
		bool	get_finished_reading() ;
		void	parse_request(char *data, int rb);
		bool	is_parsing_finished();
		// Access parsed request for routing/response building.
		Request& get_request();
		// Const accessor for request data during response generation.
		const Request& get_request() const;
		// Store serialized response for async send.
		// Store serialized response headers; if Response indicates a file, open it for streaming.
		void	set_response(const std::string& data);
		void	set_response_from_response(const Response& res);
		// Read-only response buffer.
		const std::string& get_response() const;
		// Bytes already sent from response buffer.
		size_t	get_bytes_sent() const;
		// Advance the sent offset after a successful send().
		void	add_bytes_sent(size_t n);
		// Reset send offset for a new response.
		void	reset_bytes_sent();

		// Upload streaming helpers
		bool	begin_upload(const std::string& upload_dir, const std::string& request_path,
				const std::string& content_type, size_t max_size);
		bool	flush_upload_pending();
		bool	upload_in_progress() const;
		bool	upload_done() const;
		int		get_upload_error_code() const;
		const std::string& get_upload_url() const;
		void	reset_upload_state();

		// BodySink implementation
		virtual int onBodyData(const char* data, size_t len);
		virtual void onBodyEnd();

		// Streaming accessors
		bool	is_streaming_file() const;
		int	get_file_fd() const;
		off_t	get_file_offset() const;
		void	set_file_offset(off_t off);
		size_t	get_file_size() const;
};

# endif