#! /usr/bin/python3
from http import cookies
import os
import time
import hashlib
import pickle
import sys
from urllib.parse import parse_qs

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
SESSIONS_DIR = os.path.join(SCRIPT_DIR, 'sessions')
USER_DATABASE_PATH = os.path.join(SCRIPT_DIR, 'user_database')

def writeHtmlResponse(body, status=None, extra_headers=None):
    header_lines = []
    if status is not None:
        header_lines.append("Status: " + status)
    if extra_headers:
        for key, value in extra_headers:
            header_lines.append(key + ": " + value)
    header_lines.append("Content-Type: text/html")
    header_lines.append("Content-Length: " + str(len(body)))
    sys.stdout.write("\r\n".join(header_lines) + "\r\n\r\n")
    if body:
        sys.stdout.write(body)

def getFormValue(data, key):
    values = data.get(key)
    if not values:
        return None
    return values[0]

def readFormData():
    method = os.environ.get("REQUEST_METHOD", "GET").upper()
    if method == "POST":
        content_length = os.environ.get("CONTENT_LENGTH", "0")
        try:
            length = int(content_length)
        except ValueError:
            length = 0
        raw = sys.stdin.read(length) if length > 0 else sys.stdin.read()
    else:
        raw = os.environ.get("QUERY_STRING", "")
    return parse_qs(raw, keep_blank_values=True)

class Session:
    def __init__(self, name):
        self.name = name
        self.sid = hashlib.sha1(str(time.time()).encode("utf-8")).hexdigest()
        with open(os.path.join(SESSIONS_DIR, 'session_' + self.sid), 'wb') as f:
            pickle.dump(self, f)
    def getSid(self):
        return self.sid

""" Stores Users and thier data  """
class UserDataBase:
    def __init__(self):
        self.user_pass = {}
        self.user_firstname = {}
    def addUser(self, username, password, firstname):
        self.user_pass[username] = password
        self.user_firstname[username] = firstname
        with open(USER_DATABASE_PATH, 'wb') as f:
            pickle.dump(self, f)


def printAccPage(session):
    body = "".join([
        "<html>",
        "<head><title>Account Page</title></head>",
        "<body>",
        "<h1>Welcome Again " + session.name + " !</h1>",
        "<p>Your Session ID is: " + session.getSid() + "</p>",
        "</body>",
        "<a href=\"/index.html\"> Click here to go back to homepage </a>",
        "</html>",
    ])
    writeHtmlResponse(body)

def printUserMsg(msg):
    body = "".join([
        "<html>",
        "<head><title>USER MSG</title></head>",
        "<body>",
        "<h1>" + msg + "</h1>",
        "</body>",
        "<a href=\"/pages/login.html\"> Click here to go back to login page </a>",
        "</html>",
    ])
    writeHtmlResponse(body)

def printLogin():
    body = "".join([
        "<html>",
        "<head>",
        "<meta charset=\"UTF-8\" name=\"viewport\" content=\"width=device-width, initial-scale=1\">",
        "<link rel=\"stylesheet\" href=\"/assets/css/accstyle.css\">",
        "<title> Login Page </title>",
        "</head>",
        "<body>",
        "<center> <h1> Webserve Login Form </h1> </center>",
        "<form action=\"/cgi-bin/acc.py\" method=\"post\">",
        "<div class=\"container\">",
        "<label>Username : </label>",
        "<input type=\"text\" placeholder=\"Enter Username\" name=\"username\" required>",
        "<label>Password : </label>",
        "<input type=\"password\" placeholder=\"Enter Password\" name=\"password\" required>",
        "<button type=\"submit\">Login</button>",
        "No Account?<a href=\"/pages/register.html\"> Register Here </a>",
        "</div>",
        "</form>",
        "</body>",
        "</html>",
    ])
    writeHtmlResponse(body)




def authUser(name, password):
    if os.path.exists(USER_DATABASE_PATH):
        with open(USER_DATABASE_PATH, 'rb') as f:
            database = pickle.load(f)
            if name in database.user_pass and database.user_pass[name] == password:
                session = Session(database.user_firstname[name])
                return session
            else:
                return None
    else:
        return None

def handleLogin():
    username = getFormValue(form, 'username')
    password = getFormValue(form, 'password')
    firstname = getFormValue(form, 'firstname')
    if username == None:
        printLogin()
    elif firstname == None:
        session = authUser(getFormValue(form, 'username'), getFormValue(form, 'password'))
        if(session == None):
            printUserMsg("Failed To Login, Username or Passowrd is wrong!")
        else:
            print("Correct Crenditales :D",file=sys.stderr)
            cookie_header = "SID=" + session.getSid() + "; Path=/; HttpOnly; SameSite=Lax; Max-Age=120"
            writeHtmlResponse("", status="302 Found", extra_headers=[
                ("Set-Cookie", cookie_header),
                ("Location", "/cgi-bin/acc.py"),
            ])
    else :
        if os.path.exists(USER_DATABASE_PATH):
            with open(USER_DATABASE_PATH, 'rb') as f:
                database = pickle.load(f)
                if username in database.user_pass:
                    printUserMsg("Username is already Registerd !")
                else:
                    database.addUser(username, password, firstname)
                    printUserMsg("Account Registerd Successfully!")
        else:
            database = UserDataBase()
            if username in database.user_pass:
                printUserMsg("Username is already Registerd !")
            else:
                database.addUser(username, password, firstname)
                printUserMsg("Account Registerd Successfully!")

form = readFormData()
if 'HTTP_COOKIE' in os.environ: 
    cookies = cookies.SimpleCookie()
    cookies.load(os.environ["HTTP_COOKIE"])

    if "SID" in cookies:
        print("Your Session ID is", cookies["SID"].value,file=sys.stderr)
        with open(os.path.join(SESSIONS_DIR, 'session_' + cookies["SID"].value), 'rb') as f:
            sess = pickle.load(f)
        printAccPage(sess)
    else:
        handleLogin()    
else:
    handleLogin()
            

        



