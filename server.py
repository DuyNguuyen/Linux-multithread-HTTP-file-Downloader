import http.server
import os

class RangeRequestHandler(http.server.SimpleHTTPRequestHandler):
    def do_GET(self):
        if 'Range' in self.headers:
            self.send_partial_file()
        else:
            super().do_GET()

    def send_partial_file(self):
        path = self.translate_path(self.path)
        file_size = os.path.getsize(path)
        range_header = self.headers['Range']
        range_start, range_end = self.get_range(range_header, file_size)

        self.send_response(206)
        self.send_header('Content-Type', 'application/octet-stream')
        self.send_header('Content-Range', 'bytes {}-{}/{}'.format(range_start, range_end, file_size))
        self.send_header('Content-Length', str(range_end + 1))
        self.send_header('Accept-Ranges', 'bytes')
        self.end_headers()

        with open(path, 'rb') as f:
            f.seek(range_start)
            data = f.read(range_end - range_start + 1)
            self.wfile.write(data)

    def get_range(self, range_header, file_size):
        range_header = range_header.strip().split('=')[1]
        range_start, range_end = range_header.split('-')
        range_start = int(range_start)
        range_end = int(range_end) if range_end else file_size - 1
        return range_start, range_end

if __name__ == '__main__':
    http.server.test(HandlerClass=RangeRequestHandler)
