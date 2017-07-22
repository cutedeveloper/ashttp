import sys, socket, struct, http.client
from tqdm import tqdm

def ip2int(addr):                                                               
    return struct.unpack("!I", socket.inet_aton(addr))[0]                       


def int2ip(addr):                                                               
    return socket.inet_ntoa(struct.pack("!I", addr))                            


start = int(sys.argv[1])
end = int(sys.argv[2])

for i in tqdm(range(start, end)):
    cnt = http.client.HTTPConnection(int2ip(i), 80, timeout=2)
    headers = {"Host" : "ashiyane.ir", "request_stream" : "Accept: */*", "request_stream" : "Connection: close"}
    try:
        cnt.connect()
        cnt.request("GET", "/themedata/ashiyane_05.png", "", headers)
        res = cnt.getresponse()

        if res.status == 200:
            print(int2ip(i))
    except:
        pass
    
