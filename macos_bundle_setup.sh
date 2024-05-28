framework_change_pos() {
	echo "install_name_tool -change @rpath/../Frameworks/${2}.framework/Versions/A/${2} @executable_path/../Frameworks/${2}.framework/Versions/A/${2} ${1}"
    install_name_tool -change @rpath/../Frameworks/${2}.framework/Versions/A/${2} @executable_path/../Frameworks/${2}.framework/Versions/A/${2} ${1}
}

framework_change_pos ${1} 'freetype'
framework_change_pos ${1} 'OpenAL'
framework_change_pos ${1} 'vorbisenc'
framework_change_pos ${1} 'vorbisfile'
framework_change_pos ${1} 'FLAC'
framework_change_pos ${1} 'vorbis'
framework_change_pos ${1} 'ogg'

