    #!/usr/bin/python3
    import os, sys, time, uuid

    HERE = os.path.dirname(os.path.abspath(__file__))
    STORE = os.path.join(HERE, ".sessions")
    try:
        os.makedirs(STORE, exist_ok=True)
    except OSError:
        pass

    def get_cookie(name):
        raw = os.environ.get("HTTP_COOKIE", "")
        for part in raw.split(";"):
            part = part.strip()
            if part.startswith(name + "="):
                return part[len(name) + 1:]
        return None

    sid = get_cookie("DEMOSESSION")
    new = False
    if not sid or not sid.isalnum():
        sid = uuid.uuid4().hex
        new = True

    path = os.path.join(STORE, sid)
    count = 0
    try:
        with open(path) as f:
            count = int(f.read().strip() or "0")
    except (IOError, ValueError):
        count = 0
    count += 1
    try:
        with open(path, "w") as f:
            f.write(str(count))
    except IOError:
        pass

    sys.stdout.write("Content-Type: text/plain; charset=utf-8\r\n")
    if new:
        sys.stdout.write("Set-Cookie: DEMOSESSION=%s; Path=/; HttpOnly; Max-Age=3600\r\n" % sid)
    sys.stdout.write("\r\n")
    sys.stdout.write("Session id : %s\n" % sid)
    sys.stdout.write("New session: %s\n" % ("yes (cookie just set)" if new else "no (existing cookie)"))
    sys.stdout.write("Visits     : %d\n" % count)
    sys.stdout.write("Server time: %s\n" % time.strftime("%H:%M:%S"))
