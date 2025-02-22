

DROP TABLE IF EXISTS user;
CREATE TABLE user (
  username varchar(255),
  password varchar(30),
  type_user varchar(255) not null,
  CONSTRAINT usr_type_check CHECK (type_user IN ('student', 'faculty', 'gs','sysadmin')),

  PRIMARY KEY (username)
);

DROP TABLE IF EXISTS student;
CREATE TABLE student (
    student_id         char(8) not null,
    student_email varchar(255) not null,
    student_username varchar(255) not null,
    fname    varchar(255) not null, 
    lname     varchar(25) not null,
    address     varchar(255) not null,
    degree       varchar(255) not null,
    department    varchar(255) not null,

    PRIMARY KEY (student_id),
    FOREIGN KEY (student_username) REFERENCES user(username),
    FOREIGN KEY (degree) REFERENCES program(pname),
    FOREIGN KEY (department) REFERENCES department(dname)
);

DROP TABLE IF EXISTS faculty;
CREATE TABLE faculty (
    fac_id      char(8),
    fac_username varchar(255) not null,
    fac_email varchar(255) not null,
    fname    varchar(255) not null, 
    lname     varchar(25) not null,
    department    varchar(255) not null,

    PRIMARY KEY (fac_id),
    FOREIGN KEY (fac_username) REFERENCES user(username),
    FOREIGN KEY (department) REFERENCES department(dname)
);

DROP TABLE IF EXISTS grad_sec;
CREATE TABLE grad_sec (
    grad_sec_id      char(8),
    grad_sec_email varchar(255) not null,
    grad_sec_username varchar(255),
    fname    varchar(255) not null, 
    lname     varchar(25) not null,

    PRIMARY KEY (grad_sec_id),
    FOREIGN KEY (grad_sec_username) REFERENCES user(username)
);

DROP TABLE IF EXISTS sys_admin;
CREATE TABLE sys_admin (
    sys_admin_id      char(8),
    sys_admin_email varchar(255) not null,
    sys_admin_username varchar(255),
    fname    varchar(255) not null, 
    lname     varchar(25) not null,

    PRIMARY KEY (sys_admin_id),
    FOREIGN KEY (sys_admin_username) REFERENCES user(username)
);

DROP TABLE IF EXISTS takes;
CREATE TABLE takes (
    grade  varchar(2) DEFAULT 'IP' NOT NULL,
    crn int,
    semester varchar(6),
    section int DEFAULT 1,
    year int,
    student_id char(8), 

    PRIMARY KEY (crn, semester, year, section, student_id),
    FOREIGN KEY (crn) REFERENCES course(crn),
    FOREIGN KEY (student_id) REFERENCES student(student_id)
  );

DROP TABLE IF EXISTS teaches;
CREATE TABLE teaches (
    fac_id char(8), 
    crn int,
    semester varchar(6),
    year int,
    section int DEFAULT 1,

    PRIMARY KEY (crn, semester, year, section, fac_id),
    FOREIGN KEY (crn) REFERENCES course(crn),
    FOREIGN KEY (fac_id) REFERENCES faculty(fac_id)
  );

DROP TABLE IF EXISTS course;
CREATE TABLE course (
  department char(4), 
  course_num  char(4),
  crn int,
  credits int,
  course_title varchar(255),
    
  PRIMARY KEY (crn),
  FOREIGN KEY (department) REFERENCES department(dname)
);

DROP TABLE IF EXISTS event;
CREATE TABLE event (
  crn int,
  semester varchar(6),
  year int,
  section int DEFAULT 1, 
  starttime char(8), 
  endtime char(8), 
  day_of_week varchar(9),
  building varchar(32), 
  room_number varchar(8), 
  CONSTRAINT day_type_check CHECK (day_of_week IN ('M','T','W','R','F')),

  PRIMARY KEY (crn, semester, year, section)
  FOREIGN KEY (crn) REFERENCES course(crn)
);

DROP TABLE IF EXISTS classroom;
CREATE TABLE classroom (
  building varchar(32),
  room_number varchar(8),
  room_capacity int,

  PRIMARY KEY (building, room_number)
);

DROP TABLE IF EXISTS program;
CREATE TABLE program (
    pname varchar(25),

    PRIMARY KEY (pname)
);

DROP TABLE IF EXISTS department;
CREATE TABLE department (
    dname char(4),

    PRIMARY KEY (dname)
);

DROP TABLE IF EXISTS prereq;
CREATE TABLE prereq (
    course_crn int,
    prereq_crn int,

    PRIMARY KEY (course_crn, prereq_crn),
    FOREIGN KEY (course_crn) REFERENCES course(crn),
    FOREIGN KEY (prereq_crn) REFERENCES course(crn)
  );


  --INSERTS BEYOND THIS POINT

  --user Insert
  INSERT INTO user (username,password,type_user) VALUES 
  ('billie','billiepass','student'),
  ('diana','dianapass','student'),
  ('oliver','oliverpass','gs'),
  ('narahari','naraharipass','faculty'),
  ('choi','choipass','faculty');

  --student Insert
  INSERT INTO student (student_id, student_username, student_email, fname, lname, address, degree, department) 
  VALUES 
  ('88888888', 'billie', 'billie@mail.com', 'Billie', 'Holiday', 'Billie House', 'Masters', 'CSCI'),
  ('99999999', 'diana', 'diana@mail.com', 'Diana', 'Krall', 'Diana House', 'Masters', 'CSCI');

  --faculty Insert
  INSERT INTO faculty (fac_id, fac_username, fac_email, fname, lname, department) 
  VALUES 
  ('33333333', 'narahari', 'narahari@mail.com', 'NarahariFName', 'Narahari', 'CSCI'),
  ('44444444', 'choi', 'choi@mail.com', 'ChoiFName', 'Choi', 'CSCI');

  --grad_sec Insert
  INSERT INTO grad_sec (grad_sec_id, grad_sec_username, grad_sec_email, fname, lname) 
  VALUES ('22222222', 'oliver', 'oliver@mail.com', 'Oliver', 'Noah');

  --TEST USER INSERT
  INSERT INTO user (username,password,type_user) VALUES ('testusersys','testpasssys','sysadmin');
  INSERT INTO sys_admin (sys_admin_id, sys_admin_username, sys_admin_email, fname, lname) 
  VALUES ('11111111', 'testusersys', 'testusersys@testuser.com', 'testfnamesys', 'testlnamesys');

--teaches Insert
  
INSERT INTO teaches (fac_id, crn, semester, year, section) 
VALUES
('33333333', 2, 'Spring', 2024, 1), 
('44444444', 3, 'Spring', 2024, 1);

--takes Insert
INSERT INTO takes (grade,crn,semester,section,year,student_id)
VALUES
('IP',2,'Spring',1,2024,'88888888'),
('IP',3,'Spring',1,2024,'88888888');

  --Program Insert
  INSERT INTO program (pname) VALUES 
  ('PhD'),
  ('Masters');

  --Department Insert
  INSERT INTO department (dname) VALUES
  ('CSCI'),
  ('ECE'),
  ('MATH'),
  ('PHYS');


INSERT INTO course (crn, department, course_num , credits, course_title)
VALUES 
(1,'CSCI', '6221', 3, 'SW Paradigms'),
(2,'CSCI', '6461', 3, 'Computer Architecture'),
(3,'CSCI', '6212', 3, 'Algorithms'),
(4,'CSCI', '6232', 3, 'Networks 1'),
(5,'CSCI', '6233', 3, 'Networks 2'),
(6,'CSCI', '6241', 3, 'Database 1'),
(7,'CSCI', '6242', 3, 'Database 2'),
(8,'CSCI', '6246', 3, 'Compilers'),
(9,'CSCI', '6251', 3, 'Cloud Computing'),
(10,'CSCI', '6254', 3, 'SW Engineering'),
(11,'CSCI', '6260', 3,  'Multimedia'),
(12,'CSCI', '6262', 3, 'Graphics 1'),
(13,'CSCI', '6283', 3, 'Security 1'),
(14,'CSCI', '6284', 3, 'Cryptography'),
(15,'CSCI', '6286', 3, 'Network Security'),
(16,'CSCI', '6384', 3, 'Cryptography 2'),
(17,'ECE', '6241',3,'Communication Theory'),
(18,'ECE', '6242',2,'Information Theory'),
(19,'MATH', '6210',2,'Logic'),
(20,'CSCI', '6339',3, 'Embedded Systems'),
(21, 'CSCI', '6325',3, 'Algorithms 2'),
(22, 'CSCI', '6220', 3, 'Machine Learning');

INSERT INTO event (crn, semester, year, section, starttime, endtime, day_of_week)
VALUES 
(1, 'Spring', 2024, 1, '15:00:00', '17:30:00', 'M'),
(2, 'Spring', 2024, 1, '15:00:00', '17:30:00', 'T'),
(3, 'Spring', 2024, 1, '15:00:00', '17:30:00', 'W'),
(4, 'Spring', 2024, 1, '18:00:00', '20:30:00', 'M'),
(5, 'Spring', 2024, 1, '18:00:00', '20:30:00', 'T'),
(6, 'Spring', 2024, 1, '18:00:00', '20:30:00', 'W'),
(7, 'Spring', 2024, 1, '18:00:00', '20:30:00', 'R'),
(8, 'Spring', 2024, 1, '15:00:00', '17:30:00', 'T'),
(9, 'Spring', 2024, 1, '18:00:00', '20:30:00', 'M'),
(10, 'Spring', 2024, 1, '15:30:00', '18:00:00', 'M'),
(11, 'Spring', 2024, 1, '18:00:00', '20:30:00', 'R'),
(12, 'Spring', 2024, 1, '18:00:00', '20:30:00', 'W'),
(13, 'Spring', 2024, 1, '18:00:00', '20:30:00', 'T'),
(14, 'Spring', 2024, 1, '18:00:00', '20:30:00', 'M'),
(15, 'Spring', 2024, 1, '18:00:00', '20:30:00', 'W'),
(16, 'Spring', 2024, 1, '15:00:00', '17:30:00', 'W'),
(17, 'Spring', 2024, 1, '18:00:00', '20:30:00', 'M'),
(18, 'Spring', 2024, 1, '18:00:00', '20:30:00', 'T'),
(19, 'Spring', 2024, 1, '18:00:00', '20:30:00', 'W'),
(20, 'Spring', 2024, 1, '16:00:00', '18:30:00', 'R'),
(1, 'Fall', 2024, 1, '15:00:00', '17:30:00', 'M'),
(2, 'Fall', 2024, 1, '15:00:00', '17:30:00', 'T'),
(3, 'Fall', 2024, 1, '15:00:00', '17:30:00', 'W'),
(4, 'Fall', 2024, 1, '18:00:00', '20:30:00', 'M'),
(5, 'Fall', 2024, 1, '18:00:00', '20:30:00', 'T'),
(6, 'Fall', 2024, 1, '18:00:00', '20:30:00', 'W'),
(7, 'Fall', 2024, 1, '18:00:00', '20:30:00', 'R'),
(8, 'Fall', 2024, 1, '15:00:00', '17:30:00', 'T'),
(9, 'Fall', 2024, 1, '18:00:00', '20:30:00', 'M'),
(10, 'Fall', 2024, 1, '15:30:00', '18:00:00', 'M'),
(11, 'Fall', 2024, 1, '18:00:00', '20:30:00', 'R'),
(12, 'Fall', 2024, 1, '18:00:00', '20:30:00', 'W'),
(13, 'Fall', 2024, 1, '18:00:00', '20:30:00', 'T'),
(14, 'Fall', 2024, 1, '18:00:00', '20:30:00', 'M'),
(15, 'Fall', 2024, 1, '18:00:00', '20:30:00', 'W'),
(16, 'Fall', 2024, 1, '15:00:00', '17:30:00', 'W'),
(17, 'Fall', 2024, 1, '18:00:00', '20:30:00', 'M'),
(18, 'Fall', 2024, 1, '18:00:00', '20:30:00', 'T'),
(19, 'Fall', 2024, 1, '18:00:00', '20:30:00', 'W'),
(20, 'Fall', 2024, 1, '16:00:00', '18:30:00', 'R');

INSERT INTO prereq (course_crn, prereq_crn) VALUES
(5, 4),
(7, 6),
(8, 2),
(8, 3),
(9, 2),
(10, 1),
(13, 3),
(14, 3),
(15, 13),
(15, 4),
(21, 3),
(20, 2),
(20, 3),
(16, 14);
