src = "z:\"

src = src + "\Welcome to the official CD PROJEKT RED Twitch channel!-v302423092.mp4"
src = ffmpegsource2(src)
alphabet = ImageSource("alphabet.png")

offset=17188+424-100
src = src.Trim(offset,1834151+17188)

LoadPlugin(".\TextDecode.dll")
TextDecode(src, alphabet, offset, 10)
