#pragma once

#include <AK/String.h>
#include <AK/Error.h>
#include <AK/Vector.h>
#include <AK/Weakable.h>
#include <AK/OwnPtr.h>

#include <stdio.h>

class PasswdFile : public Weakable<PasswdFile> {
public:
    ErrorOr<OwnPtr<PasswdFile>> static open_or_create();
    explicit PasswdFile(FILE*);

    static PasswdFile* the();

    struct User {
        String username;
        String password;
        int id;
        int group_id;
        String gecos;
        String home_directory;
        String shell;
    };

    Vector<User> const& users() { return m_users; }
    ErrorOr<bool> add_user(User const&);

    int avaliable_system_id();

    ~PasswdFile();
private:
    FILE* m_inner;
    Vector<User> m_users;
};