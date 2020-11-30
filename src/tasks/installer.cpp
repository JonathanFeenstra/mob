#include "pch.h"
#include "tasks.h"

namespace mob::tasks
{

installer::installer()
	: basic_task("installer")
{
}

bool installer::prebuilt()
{
	return false;
}

std::string installer::version()
{
	return {};
}

fs::path installer::source_path()
{
	return modorganizer::super_path() / "installer";
}

void installer::do_clean(clean c)
{
	if (is_set(c, clean::reclone))
		git_wrap::delete_directory(cx(), source_path());

	if (is_set(c, clean::rebuild))
		op::delete_directory(cx(), conf().path().install_installer());
}

void installer::do_fetch()
{
	const std::string repo = "modorganizer-Installer";

	run_tool(make_git()
		.url(make_git_url(task_conf().mo_org(), repo))
		.branch(task_conf().mo_branch())
		.root(source_path()));
}

void installer::do_build_and_install()
{
	run_tool<iscc>(source_path() / "dist" / "MO2-Installer.iss");
}

}	// namespace
