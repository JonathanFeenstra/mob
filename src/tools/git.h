#pragma once

namespace mob {

    namespace details {
        using git_url_pattern_format_string =
            std::format_string<std::string const&, std::string const&>;
    }

    // wrapper around git commands used by the git tool below or various `mob git`
    // commands
    //
    class git_wrap {
    public:
        // path to the git binary
        //
        static fs::path binary();

        // runs git commands in the given root directory
        //
        // `runner` is used when the commands are issued by the git tool below,
        // they ask the runner to run the various processes instead of running them
        // directly; this makes logs use the name of the task running the tool, etc.
        //
        git_wrap(fs::path root, basic_process_runner* runner = nullptr);

        // runs `git clone` with the url and branch, adds `--depth 1` when `shallow`
        // is true
        //
        void clone(const mob::url& url, const std::string& branch, bool shallow);

        // runs `git pull` with the given url and branch
        //
        void pull(const mob::url& url, const std::string& branch);

        // runs `git config` to set "user.name" and "user.email"
        //
        void set_credentials(const std::string& username, const std::string& email);

        // 1) renames the "origin" remote to "upstream",
        // 2) sets the "upstream" push url to "nopushurl" of `no_push_upstrea` is
        //    true
        // 3) adds a new "origin" remote from github with the given org and key
        // 4) sets the "origin" remote as the default push remote if
        //    `push_default_origin` is true
        //
        // this is used when cloning a repo: "origin" is usually from the
        // ModOrganizer2 org, but most devs have their own fork in which they
        // develop, so "origin" becomes "upstream" and a new "origin" remote is
        // created for their own repo
        //
        // if there's already a remote named "upstream", this is a no-op
        //
        void set_origin_and_upstream_remotes(std::string org, std::string key,
                                             bool no_push_upstream,
                                             bool push_default_origin);

        // finds all the .ts files in the root (recursive) and either sets or
        // removes the --assume-unchanged flag on all of them
        //
        // .ts files are translation files that are automatically generated by Qt
        // when building the various projects and they can change at any time;
        // pushing them creates unnecessary merge conflicts for other devs, and it's
        // a pita when it happens
        //
        // this basically ignores .ts completely when pushing: they won't be shown
        // as modified and won't be pushed if they've changed
        //
        void ignore_ts(bool b);

        // finds all the .ts files in the root (recursive) and reverts them (does
        // a `git checkout` on all of them)
        //
        // this is used when pulling changes to revert all the .ts before pulling
        // so there are no conflicts
        //
        void revert_ts();

        // returns whether the given file is known to git
        //
        bool is_tracked(const fs::path& file);

        // returns whether the given remote name exists
        //
        bool has_remote(const std::string& name);

        // adds a remote from github, no-op if it already exists
        //
        // remote_name:  name of the new remote
        //
        // org:          organization on github
        //
        // key:          path to a putty key, may be empty
        //
        // push_default: whether this remote should be the default for push, sets
        //               the remote.pushdefault config
        //
        // url_pattern:  the url pattern for the remote, should be a format string
        //               with two {} for org and git file respectively; if empty,
        //               defaults to default_github_url_pattern in git.cpp
        //
        // git_file:     the name of the git file on github, such as
        //               "modorganizer.git"; if empty, defaults to the git file used
        //               by the "origin" remote
        //
        //               this is necessary in some operations like in
        //               set_origin_and_upstream_remotes() because the "origin"
        //               remote might not exist at that point
        //
        void add_remote(
            const std::string& remote_name, const std::string& org,
            const std::string& key, bool push_default,
            std::optional<details::git_url_pattern_format_string> url_pattern = {},
            std::optional<std::string> git_file                               = {});

        // renames remote `from` to `to`
        //
        void rename_remote(const std::string& from, const std::string& to);

        // sets the push url of the given remote
        //
        void set_remote_push(const std::string& remote, const std::string& url);

        // runs `git config key value`
        //
        void set_config(const std::string& key, const std::string& value);

        // sets --assume-unchanged or --no-assume-unchanged for the given file
        //
        void set_assume_unchanged(const fs::path& relative_file, bool on);

        // returns the .git file used by the origin remote, such as modorganizer.git
        //
        std::string git_file();

        // runs `git init`
        //
        void init_repo();

        // runs `git apply` and feeds the given string as stdin; used to apply a
        // PR diff downloaded for github, for example
        //
        void apply(const std::string& diff);

        // runs `git fetch remote branch`
        //
        void fetch(const std::string& remote, const std::string& branch);

        // runs `git checkout what`
        //
        void checkout(const std::string& what);

        // runs `git submodule add` for the given branch submodule and url
        //
        void add_submodule(const std::string& branch, const std::string& submodule,
                           const mob::url& url);

        // returns the output of `git branch --show-current`, which is the name of
        // the active branch
        //
        std::string current_branch();

        // whether the root directory given in the constructor is a valid git repo
        //
        bool is_git_repo();

        // whether the repo has uncommitted changes (basically checks `git status`);
        // see delete_directory() below
        //
        bool has_uncommitted_changes();

        // whether the repo has stashed changes (checks `git stash show`); see
        // delete_directory() below
        //
        bool has_stashed_changes();

        // used by various tasks to delete a directory that was created by pulling
        // from git
        //
        // if the directory has uncommitted or stashed changes, it will output an
        // error and bail out; if not, the directory is deleted normally with
        // op::delete_directory()
        //
        static void delete_directory(const context& cx, const fs::path& dir);

        // runs `git ls-remote` to check if the repo at the url has the given branch
        // name
        //
        // used mostly by `mob release official` when given a branch name to make
        // sure the branch exists in all repos before starting the build so it
        // doesn't fail in the middle
        //
        static bool remote_branch_exists(const mob::url& u, const std::string& name);

    private:
        // git root directory, from constructor
        fs::path root_;

        // optional tool that's running these git commands
        basic_process_runner* runner_;

        // either runs the given process directly or asks runner_ to run it if it's
        // not null
        //
        // both versions are needed because run() can be called with a temporary
        // process object sometimes
        //
        int run(process&& p);
        int run(process& p);

        // log context, either gcx() or the one from runner_ if it's not null
        //
        const context& cx();
    };

    // tool to handle git operations, used by tasks
    //
    class git : public basic_process_runner {
    public:
        // what run() should do
        //
        enum ops {
            // clones the repo
            clone = 1,

            // pulls the repo
            pull,

            // pulls if the repo exists, clones otherwise
            clone_or_pull
        };

        git(ops o);

        // url to clone or pull from
        //
        git& url(const mob::url& u);

        // root directory of the git repo
        //
        git& root(const fs::path& dir);

        // branch to clone or pull
        //
        git& branch(const std::string& name);

        // whether all .ts files should be marked as --assume-unchanged when cloning
        //
        git& ignore_ts_on_clone(bool b);

        // whether all .ts files should be reverted when pulling
        //
        git& revert_ts_on_pull(bool b);

        // if this is called, sets "user.name" and "user.email" when cloning
        //
        git& credentials(const std::string& username, const std::string& email);

        // if true, clones with `--depth 1`
        //
        git& shallow(bool b);

        // if set, calls git_wrap::set_origin_and_upstream_remotes()
        //
        git& remote(std::string org, std::string key, bool no_push_upstream,
                    bool push_default_origin);

    protected:
        void do_run() override;

    private:
        // operation
        ops op_;

        // set by the various functions above
        mob::url url_;
        fs::path root_;
        std::string branch_;
        bool ignore_ts_;
        bool revert_ts_;
        std::string creds_username_;
        std::string creds_email_;
        bool shallow_;
        std::string remote_org_;
        std::string remote_key_;
        bool no_push_upstream_;
        bool push_default_origin_;

        bool do_clone();
        void do_pull();
    };

    // tool to handle git submodule operations, used by the modorganizer task to
    // set up the submodules
    //
    // this tool is not normally run directly, instances of git_submodule are given
    // to the git_submodule_adder, which runs all of them in a thread
    //
    class git_submodule : public basic_process_runner {
    public:
        git_submodule();

        // remote url
        //
        git_submodule& url(const mob::url& u);

        // root directory of the repo
        //
        git_submodule& root(const fs::path& dir);

        // branch name
        //
        git_submodule& branch(const std::string& name);

        // submodule name
        //
        git_submodule& submodule(const std::string& name);
        const std::string& submodule() const;

    protected:
        void do_run() override;

    private:
        mob::url url_;
        fs::path root_;
        std::string branch_;
        std::string submodule_;
    };

    // queues submodule operations with queue(), runs them in a thread because they
    // take a long time but can happen while stuff is building
    //
    class git_submodule_adder {
    public:
        // calls stop() and joins
        //
        ~git_submodule_adder();

        // only one instance, runs the thread and waits for submodules to be added
        // by queue()
        //
        static git_submodule_adder& instance();

        // adds a submodule to the queue
        //
        void queue(git_submodule g);

        // stops the thread
        //
        void stop();

    private:
        // used to sleep until queue() is called
        //
        struct sleeper {
            std::mutex m;
            std::condition_variable cv;
            bool ready = false;
        };

        // log context
        context cx_;

        // thread
        std::thread thread_;

        // queue
        std::vector<git_submodule> queue_;
        mutable std::mutex queue_mutex_;

        // true in stop(), stops the thread
        std::atomic<bool> quit_;

        // used to sleep until queue() is called
        sleeper sleeper_;

        git_submodule_adder();

        // starts the thread
        //
        void run();

        // thread function, sleeps until queue() is called
        //
        void thread_fun();

        // forces the thread function to wake up
        //
        void wakeup();

        // processes the queue
        //
        void process();
    };

}  // namespace mob
