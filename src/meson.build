gtkdep = dependency('gtk+-3.0')
ffmpeg_deps = [
    dependency('libavcodec'),
    dependency('libavformat'),
    dependency('libavutil'),
    dependency('libswscale')
  ]

executable('video_splitter',
  ['main.c', 'video_splitter.c'],
  dependencies: [gtkdep] + ffmpeg_deps,
  install: true
)
