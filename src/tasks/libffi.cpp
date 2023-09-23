#include "pch.h"
#include "tasks.h"

namespace mob::tasks {

    // required by python

    libffi::libffi() : basic_task("libffi") {}

    std::string libffi::version()
    {
        return {};
    }

    bool libffi::prebuilt()
    {
        // actually always prebuilt
        return false;
    }

    fs::path libffi::source_path()
    {
        return conf().path().build() / "libffi";
    }

    void libffi::do_clean(clean c)
    {
        // delete the whole thing
        if (is_set(c, clean::reclone))
            git_wrap::delete_directory(cx(), source_path());
    }

    void libffi::do_fetch()
    {
        run_tool(make_git()
                     .url(make_git_url("python", "cpython-bin-deps"))
                     .branch("libffi-3.4.4")
                     .root(source_path()));
    }

    fs::path libffi::include_path()
    {
        return libffi::source_path() / "amd64" / "include";
    }

    fs::path libffi::lib_path()
    {
        return libffi::source_path() / "amd64";
    }

}  // namespace mob::tasks
