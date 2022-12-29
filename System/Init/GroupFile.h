#pragma once

#include <AK/Weakable.h>
#include <AK/String.h>
#include <AK/Error.h>
#include <AK/Vector.h>
#include <AK/OwnPtr.h>

#include <stdio.h>

class GroupFile :
        public Weakable<GroupFile>
        {
public:
    ErrorOr<OwnPtr<GroupFile>> static open_or_create();
    explicit GroupFile(FILE*);

    static GroupFile* the();

    struct Group {
        String name;
        String password;
        int id;
        Vector<String> users;
    };

    Vector<Group> const& groups() { return m_groups; }
    ErrorOr<bool> add_group(Group const&);

    int avaliable_system_id();

    ~GroupFile();
private:
    FILE* m_inner;
    Vector<Group> m_groups;
};