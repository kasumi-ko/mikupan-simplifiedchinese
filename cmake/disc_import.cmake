include(FetchContent)

if(POLICY CMP0168)
    cmake_policy(SET CMP0168 NEW)
endif()

block()
    # The importer only reads raw ISO9660. Keep archive tools and optional
    # codecs out of the Windows port build.
    foreach(feature IN ITEMS
            TAR CPIO CAT UNZIP TEST INSTALL OPENSSL LIBB2 LZ4 LZMA ZSTD ZLIB
            BZip2 LIBXML2 EXPAT WIN32_XMLLITE PCREPOSIX PCRE2POSIX ACL XATTR)
        set(ENABLE_${feature} OFF)
    endforeach()
    FetchContent_Declare(mikupan_libarchive
        URL https://codeload.github.com/libarchive/libarchive/tar.gz/refs/tags/v3.8.2
        URL_HASH SHA256=97548897f06ed7f5cd847fc62a67420809dbbd9b45ec13a25d3290fafba046c8
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    )
    FetchContent_MakeAvailable(mikupan_libarchive)
    add_custom_command(TARGET MikuPan POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${mikupan_libarchive_SOURCE_DIR}/COPYING"
            "$<TARGET_FILE_DIR:MikuPan>/libarchive-LICENSE.txt"
    )
endblock()

target_sources(MikuPan PRIVATE src/mikupan/io/mikupan_disc_import.cpp)
target_compile_definitions(MikuPan PRIVATE LIBARCHIVE_STATIC)
target_link_libraries(MikuPan PRIVATE archive_static)
