#include "skroot_box_command.h"
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <malloc.h>
#include <sys/wait.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <sys/prctl.h>

#include "skroot_box_umbrella.h"
#include "skroot_box_fork_helper.h"

#include "kernel_module_kit_umbrella.h"

namespace skroot_box {
SkBoxErr get_root_proxy(const char* str_root_key) {
	if (!str_root_key) return SkBoxErr::ERR_PARAM;
	if (!strlen(str_root_key)) return SkBoxErr::ERR_PARAM;
	if(is_failed(skroot_env::get_root(str_root_key))) return SkBoxErr::ERR_NO_ROOT;
	return SkBoxErr::OK;
}

SkBoxErr run_root_cmd_with_cb(const char* str_root_key, const char* cmd, void (*cb)(const char* result)) {
	if (str_root_key == NULL || cmd == NULL || strlen(cmd) == 0) return SkBoxErr::ERR_PARAM;

	SkBoxErr err = SkBoxErr::OK;

	std::string cmd_add_err_info = cmd;
	cmd_add_err_info += " 2>&1";

	std::string result;
	fork_pipe_info finfo;
	if(fork_pipe_child_process(finfo)) {
		do {
			BREAK_IF_ERROR_SKBOX(get_root_proxy(str_root_key));
			FILE * fp = popen(cmd_add_err_info.c_str(), "r");
			if(!fp) {
				err = SkBoxErr::ERR_POPEN;
				break;
			}
			int pip = fileno(fp);
			while(true) {
				char rbuf[1024] = {0};
				ssize_t r = read(pip, rbuf, sizeof(rbuf));
				if (r == -1 && errno == EAGAIN) continue;
				else if(r > 0) { std::string str_convert(rbuf, r); result += str_convert; }
				else break;
			}
			pclose(fp);
		} while(0);
		write_errcode_from_child(finfo, err);
		write_string_from_child(finfo, result);
		finfo.close_all();
		_exit(0);
		return {};
	}

	if(!read_errcode_from_child(finfo, err)) {
		err = SkBoxErr::ERR_READ_CHILD_ERRCODE;
	} else if(!read_string_from_child(finfo, result)) {
		err = SkBoxErr::ERR_READ_CHILD_STRING;
	} else {
		cb(result.c_str());
	}
	int status = 0; waitpid(finfo.child_pid, &status, 0);
	return err;
}
}
