#!/usr/bin/env python
## History:
##
## 2016/12/30 - [JianTang] Created file
##
## Copyright (c) 2016 Ambarella, Inc.
##
## This file and its contents ("Software") are protected by intellectual
## property rights including, without limitation, U.S. and/or foreign
## copyrights. This Software is also the confidential and proprietary
## information of Ambarella, Inc. and its licensors. You may not use, reproduce,
## disclose, distribute, modify, or otherwise prepare derivative works of this
## Software or any portion thereof except pursuant to a signed license agreement
## or nondisclosure agreement with Ambarella, Inc. or its authorized affiliates.
## In the absence of such an agreement, you agree to promptly notify and return
## this Software to Ambarella, Inc.
##
## THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
## INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF NON-INFRINGEMENT,
## MERCHANTABILITY, AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
## IN NO EVENT SHALL AMBARELLA, INC. OR ITS AFFILIATES BE LIABLE FOR ANY DIRECT,
## INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
## (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
## LOSS OF USE, DATA, OR PROFITS; COMPUTER FAILURE OR MALFUNCTION; OR BUSINESS
## INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
## CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
## ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
## POSSIBILITY OF SUCH DAMAGE.
##

import os
import sys
import time
import random
import signal
import re
import threading
from xml.dom import minidom
from socket import *

LOG_PORT		= 2008
DUMP_REG_PORT	= 2009

SWITCH_ON_TIMER		= 0
SWITCH_ON_SUCCESS	= 1

threads = []
test_pid = 0
monitor_exit = False
logFileSock = None

def stopSignal(signum, frame):
	os.killpg(tester_pid, signal.SIGTERM)
	os.wait()
	print "stopsignal caught, program exit..."
	os._exit(1)

def chldSignal(signum, frame):
	if threads == []:
		return
		
	for t in threads:
		t.mutex.acquire()
		childpid = t.childpid
		t.mutex.release()
		if childpid != 0:										#has child
			status = os.waitpid(childpid, os.WNOHANG)
			if status[0] == 0:			#child alive
#				print "child %d alive" % childpid
				continue
			else:					#child dead
				t.childpid = 0
				
				if os.WIFSIGNALED(status[1]):									#child is killed
					t.mutex.acquire()
					t.cond.notify()
					t.mutex.release()
#					print "child %d is killed" % childpid
					
				elif os.WIFEXITED(status[1]) and (os.WEXITSTATUS(status[1]) == 0):		#child exited with 0
					if t.switch_mode == SWITCH_ON_SUCCESS:			#switch to next test command immediately
						t.mutex.acquire()
						t.cond.notify()
						t.mutex.release()
#					print "child %d exited with 0" % childpid
				
				elif os.WIFEXITED(status[1]) and (os.WEXITSTATUS(status[1]) != 0):	#child exited with error
					if t.switch_mode == 2:			#kill all tester and exit
						print "tester error caught, program exit..."
						os._exit(1)
					else:							#switch to next test command immediately
						t.mutex.acquire()
						t.cond.notify()
						t.mutex.release()
#					print "child %d exited with error" % childpid
	

def writeFileToPS(port, filename):
#	print 'write file to PS:', port, filename
	HOST = '10.0.0.1'
	filenameBuf = ''
	
	ADDR = (HOST, port)
	filenameBuf = filenameBuf.zfill(256)
	filename = filename + '\0' + filenameBuf[len(filename) + 1:]
	
	tcpCliSock = socket(AF_INET, SOCK_STREAM)
	tcpCliSock.settimeout(2)
	try:
		tcpCliSock.connect(ADDR)
	except Exception, e:
		print "connect", e
		return -1
	
	tcpCliSock.send(filename) 
	return tcpCliSock

def createLogFile():
	global logFileSock;
	logFileName = 'media/test_ipcam.log'
	logFileSock = writeFileToPS(LOG_PORT, logFileName)
	if logFileSock < 0:
		print 'cannot connect to Port Sever. Now run without log...'

def dump_reg():
	reg_info = 	(
				('0x70104000', '0x3c', 'reg_0x70104000_0x7010403c.dat'), 
				('0x70150000', '0x50', 'reg_0x70150000_0x70150050.dat'), 
				('0x70150800', '0x800', 'reg_0x70150800_0x70150fff.dat'),
				('0x70160000', '0x50', 'reg_0x70160000_0x70160050.dat'),
				('0x70160200', '0x200', 'reg_0x70160200_0x701603ff.dat'),
				('0xc0080000', '0x20000', 'reg_0xc0080000_0xc00a0000.dat')
				)
	
	for item in reg_info:
		cmdLine = 'amba_debug -r ' + item[0] + ' -s ' + item[1] + ' -f stdout'
		f = os.popen(cmdLine)	
		filename = 'media/' + item[2]
		regDumpSock = writeFileToPS(DUMP_REG_PORT, filename)
		if regDumpSock < 0:
			print 'cannot connect to Port Sever. dump_reg failed!'
			return -1
		regDumpSock.send(f.read(-1))
		regDumpSock.close()
		
	return 0

def dump_reg2():
	reg_info = 	('0xc0080000', '0x20000', 'reg_0xc0080000_0xc00a0000')
	regdat = '/tmp/' + reg_info[2] + '.dat'
	regdump = '/tmp/' + reg_info[2] + '.txt'
	firmware = '/lib/firmware/'
	filename = 'media/' + reg_info[2] + '.txt'
	
	cmdLine = 'amba_debug -r %s -s %s -f %s' % (reg_info[0], reg_info[1], regdat)
	os.system(cmdLine)
	
	cmdLine = 'dsplog2 %s %s > %s' % (regdat, firmware, regdump)
	os.system(cmdLine)
	
	regDumpSock = writeFileToPS(DUMP_REG_PORT, filename)
	if regDumpSock < 0:
		print 'cannot connect to Port Sever. dump_reg failed!'
		return -1
		
	f = open(regdump, 'r')
	regDumpSock.send(f.read(-1))
	regDumpSock.close()
	
	return 0
		
class MyThread(threading.Thread):
	def __init__(self, func, args, name=''):
		threading.Thread.__init__(self)
		self.name = name
		self.func = func
		self.args = args
		self.mutex = threading.Lock()
		self.cond = threading.Condition(self.mutex)
		self.childpid = 0
		
	def run(self):
		self.res = self.func(self, *self.args)
		
def test_txt(self, fobj):
	first_run = 1
	frame_randstart = 100
	frame_randstop = 100
	frame_randstart = 0
	frame_randstop = 0
	frame_randstep = 0
	mode = 'rad'
	round = 0
	lineIndex = 0
	turn = 0
            
	allLines = fobj.readlines()
	fobj.close()

	optionLine = allLines[0]
	cmdLines = allLines[1:]
	
	# parse option and test cmdline
	m = re.findall('--(\w+)\s+((\w+,)*\w+)', optionLine)
	for eachOpt in m:
		if eachOpt[0] == 'time_randrange':
			[time_randstart, time_randstop] = [int(e) for e in eachOpt[1].split(',')]
		elif eachOpt[0] == 'mode':
			mode = eachOpt[1]
		elif eachOpt[0] == 'round':
			round = int(eachOpt[1])
		elif eachOpt[0] == 'switch_mode':
			self.switch_mode = int(eachOpt[1])
		elif eachOpt[0] == 'frame_randrange':
			[frame_randstart, frame_randstop, frame_randstep] = [int(e) for e in eachOpt[1].split(',')]
			
	print 'mode:', mode, ' round:', round, ' switch_mode:', self.switch_mode
	print 'time_randrange:', time_randstart, time_randstop
	print 'frame_randrange:', frame_randstart, frame_randstop, frame_randstep

	if first_run:
		time.sleep(3)
		first_run = 0
	
	while True:
		if mode == 'rad':
			cmdLine = random.choice(cmdLines).strip()	
		elif mode == 'seq':
			if lineIndex == 0:
				turn += 1
				if round > 0 and turn > round:
					break
			cmdLine = cmdLines[lineIndex].strip()
			lineIndex = (lineIndex+1) % len(cmdLines)

		if cmdLine == '' or cmdLine.startswith(('#', '-', '\\', '\/')):
			print 'skip line'
			continue
			
# add test2 --frames option	
		if frame_randstart > 0 and frame_randstop > frame_randstart:
			frames = random.randrange(frame_randstart, frame_randstop, frame_randstep)
			cmdLine = cmdLine + ' --frames ' + str(frames)
			
		wait_sec = random.uniform(time_randstart, time_randstop)
		cmd = cmdLine.split()
		
		print '\ncmd: ', cmdLine, '\t(%f sec)' % wait_sec
		if logFileSock > 0:
			logFileSock.send(cmdLine + '\n')
		
		
		if cmdLine == 'sleep':
			print 'sleeping...'
			test_pid = 0		# do not need spawn child
		else:
			try:
				test_pid = os.spawnvp(os.P_NOWAIT, cmd[0], cmd)
			except Exception, e:
				print "spawn", e
				return -1
	#		print "child %d spawned " % test_pid
		
		self.mutex.acquire()
		self.childpid = test_pid
		self.cond.wait(wait_sec)
		self.mutex.release()

		if self.childpid != 0:
			print "kill", self.childpid
			try:
				os.kill(self.childpid, signal.SIGTERM)
			except Exception, e:
				print "kill", e
				return -1
				
			self.mutex.acquire()
			self.cond.wait()
			self.mutex.release()		


def test_xml(self, filename):	
	opt = []
	xmldoc = minidom.parse(filename)

	rand_a = int(xmldoc.getElementsByTagName('rand_a')[0].firstChild.nodeValue)
	rand_b = int(xmldoc.getElementsByTagName('rand_b')[0].firstChild.nodeValue)

	optBaseNode = xmldoc.getElementsByTagName('option')[0]
	optNodeList = [e for e in optBaseNode.childNodes if e.nodeType == e.ELEMENT_NODE]

	for e in optNodeList:
		optName = e.nodeName
		optRange = [f.firstChild.nodeValue for f in e.childNodes if f.nodeType == f.ELEMENT_NODE and f.firstChild.nodeType == e.TEXT_NODE]
		opt.append([optName, optRange])
		
	logFileName = 'media/test_ipcam.log'
	logFileSock = writeFileToPS(LOG_PORT, logFileName)
	if logFileSock < 0:
		print 'cannot connect to Port Sever. Now run without log...'
		time.sleep(3)
		
	while True:	
		cmdLine = './test2'
		for e in opt:				# -shortOpt arg or --longOpt arg
			if len(e[0]) == 1:
				cmdLine +=  ' -'
			else:
				cmdLine +=  ' --'
			cmdLine = cmdLine + e[0]
			if e[1] != []:
				cmdLine = cmdLine + ' ' + random.choice(e[1])

		cmd = cmdLine.split()
		wait_sec = random.uniform(rand_a, rand_b)
		
		print '\ncmd: ', cmdLine, '\t(%f wait_sec)' % wait_sec
		if logFileSock > 0:
			logFileSock.send(cmdLine + '\n')
		try:
			test_pid = os.spawnvp(os.P_NOWAIT, cmd[0], cmd)
	#		test_pid = os.fork()
		except Exception, e:
			print "spawn", e
			return -1

#		if test_pid == 0:
#			print 'test2', os.getpid(), os.getpgrp()
#			os.execvp(cmd[0], cmd)
#		print "child %d spawned " % test_pid
		
		self.mutex.acquire()
		self.childpid = test_pid
		self.cond.wait(wait_sec)
		self.mutex.release()

		if self.childpid != 0:
		#	print "kill", self.childpid
			try:
				os.kill(self.childpid, signal.SIGTERM)
			except Exception, e:
				print "kill", e
				return -1
				
			self.mutex.acquire()
			self.cond.wait()
			self.mutex.release()


def monitor(self, childPgid):

	fobj = open('/proc/interrupts')
	pre_line = ''
	i = 0
	start_time = time.time()
	sec_elapsed = 0
	
	while True:					#find vdsp interrupt line in /proc/interrupts
		line = fobj.readline()
		if line == '':
			fobj.seek(0, 0)
			i = 0
			continue
		m = re.search('(\d+)\s+-\s+vdsp', line)		
		if m is not None and m.group(1) != '0':
			break
		i += 1
		
	while True:

		fobj.seek(0, 0)
		lines = fobj.readlines()
		
#		if (time.time() - start_time) > 30:				#for debug			
		if pre_line == lines[i] or monitor_exit:
		
			sec_elapsed = time.time() - start_time
			if monitor_exit:
				str = 'video corrupt at time: %dh : %dm : %ds' % ((sec_elapsed/3600)%60, (sec_elapsed/60)%60, sec_elapsed%60)
			else:											#dsp crash
				os.killpg(childPgid, signal.SIGTERM)		#kill tester process-group
				os.wait()
				str = 'dsp crashed at time: %dh : %dm : %ds' % ((sec_elapsed/3600)%60, (sec_elapsed/60)%60, sec_elapsed%60)
				
			print str
			logFileName = 'media/test_ipcam.log:a'
			logFileSock = writeFileToPS(LOG_PORT, logFileName)
			if logFileSock < 0:
				print 'cannot connect to Port Sever. Crash time unsaved!'
				
			logFileSock.send(str)
			logFileSock.close()
			dump_reg2()
			os._exit(1)							#terminate program
			return -1
			
		pre_line = lines[i]
				
		time.sleep(1)

def main(argv):
	global threads
	global tester_pid
	global monitor_exit
	i = 0

	signal.signal(signal.SIGINT,stopSignal)
	signal.signal(signal.SIGCHLD,chldSignal)	
	
	if argv == []:	
		tester_filename = ['test_case.txt',]
	else:
		tester_filename = argv

	tester_pid = os.fork()
	if tester_pid == 0:
	
		for filename in tester_filename:
	
			if os.path.isfile(filename):
				pass
			elif os.path.isfile('/usr/local/bin/' + filename):
				filename = '/usr/local/bin/' + filename
			else:
				print 'No such file', filename
				return -1
			
			try:
				fobj = open(filename, 'r')
			except IOError, e:
				print 'could not open file:', e
				return 2
			
			if filename.endswith('.txt'):
				t = MyThread(test_txt, (fobj, ), test_txt.__name__)
			elif filename.endswith('.xml'):
				t = MyThread(test_xml, (filename, ), test_xml.__name__)
			else:
				print "unknow file type"
				return -1
			threads.append(t)
	
		for t in threads:
			t.setDaemon(True)
		
		os.setpgid(0, 0)		#set tester process as a process-group leader
		createLogFile()
		for t in threads:
			t.start()
#		for t in threads:
#			t.join()
		while i < len(threads):
			if not threads[i].isAlive():
				i += 1
#			time.sleep(1)
	else:
		monitor_exit = False
		t = MyThread(monitor, (tester_pid, ), monitor.__name__)
#		t.setDaemon(True)
		t.start()
		os.wait()
		monitor_exit = True
#		monitor(tester_pid)

	print 'main end.'

if __name__ == '__main__':
	try:
		main(sys.argv[1:])
	except Exception, e:
		print "main", e

