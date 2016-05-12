Debug = 1
LoadPlugin(((Debug == 1) ? "Debug" : "Release") + "\ssifSource4.dll")

pipe_name = "\\.\pipe\my_pipe_section\firstpipe"

ssReadFromPipe(pipe_name)
