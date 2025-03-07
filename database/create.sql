

DROP TABLE IF EXISTS users;
CREATE TABLE users (
    id INTEGER PRIMARY KEY,
    username TEXT UNIQUE NOT NULL,
    role TEXT CHECK(role IN ("admin", "user")) NOT NULL
);

DROP TABLE IF EXISTS articles;
CREATE TABLE articles (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    title TEXT NOT NULL UNIQUE,
    file_path TEXT NOT NULL,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    author_id INTEGER NOT NULL,
    is_published BOOLEAN DEFAULT FALSE,
    FOREIGN KEY (author_id) REFERENCES users(id)
);


INSERT INTO users (id, username, role) VALUES
(0, 'root', "admin");


INSERT INTO articles (title, file_path, author_id, is_published)
VALUES (
    "Getting Started",
    "./wiki_content/getting_started.txt",
    0,
    1
);

INSERT INTO articles (title, file_path, author_id, is_published)
VALUES (
    "Ain't the net swell?",
    "./wiki_content/motivation.txt",
    0,
    1
);




