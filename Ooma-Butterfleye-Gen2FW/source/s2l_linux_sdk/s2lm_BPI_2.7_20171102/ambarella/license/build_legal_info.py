#!/bin/env python
'''
/*******************************************************************************
 * build_legal_info.py
 *
 * History:
 *   Dec 20, 2016 - [Peng Wang] Initial version
 *
 *
 * Copyright (c) 2016 Ambarella, Inc.
 *
 * This file and its contents ("Software") are protected by intellectual
 * property rights including, without limitation, U.S. and/or foreign
 * copyrights. This Software is also the confidential and proprietary
 * information of Ambarella, Inc. and its licensors. You may not use, reproduce,
 * disclose, distribute, modify, or otherwise prepare derivative works of this
 * Software or any portion thereof except pursuant to a signed license agreement
 * or nondisclosure agreement with Ambarella, Inc. or its authorized affiliates.
 * In the absence of such an agreement, you agree to promptly notify and return
 * this Software to Ambarella, Inc.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF NON-INFRINGEMENT,
 * MERCHANTABILITY, AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL AMBARELLA, INC. OR ITS AFFILIATES BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; COMPUTER FAILURE OR MALFUNCTION; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 */
'''

import os, re, sys, shutil


class HtmlContainer:
    '''
    HTML file container, includes methods to add license info and create html
    file, invoke method create_html_file to start the build.
    '''
    def __init__(self):
        self.pkg_info = {}
        pass


    def create_html_file(self, file_path):
        self.fp = open(file_path, 'w')

        self.__write_html_header()
        self.__start_html_body()
        self.__finish_html()
        print 'Create license html file (%s)'%file_path


    def add_license_info(self, pkg_name, license_path):
        try:
            self.pkg_info[pkg_name].append(license_path)
        except:
            self.pkg_info[pkg_name] = license_path
        pass


    def __finish_html(self):
        self.fp.write("</BODY>\n</HTML>")
        self.fp.close()


    def __start_html_body(self):
        self.fp.write('<BODY>\n')
        self.fp.write('<DIV id="nav" style="width:25%;">\n')
        self.fp.write('<UL>\n')

        for (pkg_name, license_info) in sorted(self.pkg_info.items()):
            if len(license_info) == 1:
                self.__write_html_item(pkg_name, license_info[0])
                continue
            self.fp.write('    <LI>%s</LI>\n'%pkg_name)
            self.fp.write('<UL>\n')
            for per_license in license_info:
                if isinstance(per_license, str):
                    self.fp.write('    <LI><A HREF="#" ONCLICK="open_src(\'%s\');">%s</A></LI>\n'%(per_license, os.path.basename(per_license)))
                elif isinstance(per_license, list):
                    self.fp.write('    <LI><A HREF="#" ONCLICK="open_src(\'%s\');">%s (%s)</A></LI>\n'
                            %(per_license[0], os.path.basename(per_license[0]), per_license[1]))
            self.fp.write('</UL>\n')
        self.fp.write('</UL>\n')
        self.fp.write('</DIV>\n')
        self.fp.write('''<DIV id="section">
    <IFRAME id="content" style="width:100%;height:100%;position:absolute;" frameborder="0" scrolling="no" srcdoc="<H2>Ambarella SDK Third-Party Packages License Information</H2>">
    </IFRAME>
</DIV>
<DIV id="footer">
Copyright &copy www.ambarella.com
</DIV>\n''')


    def __write_html_item(self, line_name, license):
        if isinstance(license, str):
            self.fp.write('<LI><A HREF="#" ONCLICK="open_src(\'%s\');">%s</A></LI>\n'%(license, line_name))
        elif isinstance(license, list):
            self.fp.write('<LI><A HREF="#" ONCLICK="open_src(\'%s\');">%s (%s)</A></LI>\n'%(license[0], line_name, license[1]))


    def __write_html_header(self):
        self.fp.write('''<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01 //EN" "http://ww.w3.org/TR/html4/strict.dtd">
<!-- This HTML created automatically by Ambarella License script -->
<HTML>
<HEAD>
    <TITLE>Ambarella SDK Third-Party Packages License Information</TITLE>
    <STYLE>
    #nav {
        line-height:30px;
        background-color:#eeeeee;
        width:100px;
        float:left;
        padding:5px;
        }
    #section {
        width:350px;
        float:left;
        padding:10px;
        }
    #footer {
        background-color:black;
        color:white;
        clear:both;
        text-align:center;
        padding:5px;
        }
    </STYLE>
    <SCRIPT>
    function open_src(loc) {
        document.getElementById('content').srcdoc = "<iframe style='width:100%;height:100%;position:absolute;' frameborder='0' src='" +loc +"'></iframe>";
        return true
        }
    </SCRIPT>
</HEAD>\n''')


class PackageContainer:
    '''
    Container includes license information for released packages.
    '''
    def __init__(self, folder):
        # pkg_dic = { PKG_NAME: Makefile_container, ....}
        self.pkg_dic = {}
        self.__get_pkg_lst(folder)


    def __get_pkg_lst(self, folder_path):
        self.no_make_pkg_lst = []

        for per_item in os.listdir(folder_path):
            item_full_path = os.path.join(folder_path, per_item)
            if os.path.isdir(item_full_path):
                make_file = os.path.join(item_full_path, 'make.inc')
                if os.path.exists(make_file):
                    self.pkg_dic[per_item] = MakefileContainer(make_file)
                else:
                    self.no_make_pkg_lst.append(per_item)


    def scan_and_create(self, config_ctn, license_pool, output_dir):
        # create license folder under fakeroot
        html_ctn = HtmlContainer()
        html_dir = output_dir
        html_file = os.path.join(html_dir, 'license.html')
        if os.path.exists(html_dir):
            shutil.rmtree(html_dir)
        os.mkdir(html_dir)

        for (pkg, pkg_ctn) in self.pkg_dic.items():
            if pkg_ctn.is_released(config_ctn):
                license_info = license_pool.get_license_path(pkg)
                if license_info == []:
                    #print 'ERROR: Cannot find license info for package(%s).'%pkg
                    continue
                else:
                    html_ctn.add_license_info(pkg, self.__add_link_to_info(license_info))

        # create html file
        html_ctn.create_html_file(html_file)

        # copy license to fakeroot folder
        dst_general_lic = os.path.join(html_dir, 'general_copying_files')
        dst_other_lic = os.path.join(html_dir, 'other_copying_files')
        try:
            shutil.rmtree(dst_general_lic)
        except:
            pass
        try:
            shutil.rmtree(dst_other_lic)
        except:
            pass
        shutil.copytree(license_pool.general_lic_path, dst_general_lic)
        self.__create_link_files(dst_general_lic)
        shutil.copytree(license_pool.other_lic_path, dst_other_lic)
        self.__create_link_files(dst_other_lic)


    def __create_link_files(self, dir_path):
        for per_item in os.listdir(dir_path):
            item_path = os.path.join(dir_path, per_item)
            if os.path.isfile(item_path):
                (new_name, is_changed) = self.__trans_to_link_file(per_item)
                if is_changed:
                    new_path = os.path.join(dir_path, new_name)
                    if os.path.exists(new_path):
                        continue
                    try:
                        os.link(item_path, new_path)
                    except Exception as e:
                        print "Create link file(%s) error(%s)"%(new_path, str(e))
            elif os.path.isdir(item_path):
                self.__create_link_files(item_path)
        return


    def __add_link_to_info(self, license_info):
        info_with_link = []

        for per_item in license_info:
            if isinstance(per_item, list):
                temp_sub_list = [self.__trans_to_link_file(per_item[0])[0]]
                temp_sub_list.extend(per_item[1:])
                info_with_link.append(temp_sub_list)
            else:
                info_with_link.append(self.__trans_to_link_file(per_item)[0])
        return info_with_link


    def __trans_to_link_file(self, src_name):
        if '.txt' in src_name:
            return (src_name, False)

        if '.TXT' in src_name:
            return (src_name, False)

        if '.pdf' in src_name:
            return (src_name, False)

        if '.html' in src_name:
            return (src_name, False)

        return ('%s.txt'%src_name, True)


class MakefileContainer:
    '''
    Container used for Macros in makefile and judge whether the package
    is released.
    '''
    def __init__(self, file_path):
        self.macro_set_lst = []

        with open(file_path, 'r') as f:
            for line in f:
                if re.match(r'^#', line):
                    continue
                ret = re.match(r'ifeq \(\$\((\S+)\), y', line)
                if ret:
                    self.macro_set_lst.append(ret.groups()[0])


    def is_released(self, config_container):
        for item in self.macro_set_lst:
            if config_container.is_macro_set(item):
                return True
        return False


class ConfigContainer:
    '''
    SDK configure file container, used to store all Macros which have been
    selected.
    '''
    def __init__(self, config_file):
        self.macro_set_lst = []

        with open(config_file, 'r') as f:
            for line in f:
                if re.match(r'^#', line):
                    continue
                ret = re.match(r'(\S*)=y', line)
                if ret:
                    self.macro_set_lst.append(ret.groups()[0])


    def is_macro_set(self, macro):
        return True if macro in self.macro_set_lst else False


class LicensePool:
    '''
    Class include license information parsed from software_license_list.txt file.
    '''
    def __init__(self, pool_path):
        self.license_str = {}
        self.general_license_dic = {}
        self.other_license_dic = {}
        self.pool_path = pool_path

        self.__init_licence_pool_info('general_copying_files', 'other_copying_files')


    def import_from_file(self, filename):
        self.in_block = ''
        pkg_key = ''
        file_path = os.path.join(self.pool_path, filename)
        with open(file_path, 'r') as f:
            for line in f:
                if line.strip() == '':
                    self.in_block = ''
                    continue

                ret = re.match(r'^(\S+)', line)
                if ret:
                    pkg_key = ret.groups()[0]
                    self.license_str[pkg_key] = {}
                else:
                    if pkg_key == '':
                        #print 'ERROR: Cannot find package key for string(%s)'%line
                        continue
                    ret_lst = self.__parser_line(line)
                    if not ret_lst:
                        continue
                    elif ret_lst[0]:
                        self.license_str[pkg_key][ret_lst[0]] = ret_lst[1]
                    else:
                        self.license_str[pkg_key][self.in_block] += ret_lst[1]


    def get_license_path(self, pkg_name):
        try:
            license_type = self.license_str[pkg_name]['license']
        except:
            license_type = self.__smart_check(pkg_name)

        if license_type == None:
            return []

        try:
            return self.other_license_dic[pkg_name]
        except:
            return self.__general_license(license_type)


    def __general_license(self, license_type):
        license_lst = []

        for i in license_type.split(','):
            rename_str = '_'.join(i.strip().split(' '))
            try:
                license_lst.append(self.general_license_dic[rename_str])
                continue
            except:
                pass
            ret = re.match(r'([\S\s]+)\(([\S\s]+)\)', i)
            if ret:
                rename_str = '_'.join(ret.groups()[0].strip().split(' '))
                try:
                    license_lst.append([self.general_license_dic[rename_str], ret.groups()[1]])
                    continue
                except:
                    pass
            ret = re.match(r'\s*(\S+)\s+[Ll]ibrary\s+(\S+)', i)
            if ret:
                rename_str = '%s_LIB_%s'%(ret.groups()[0], ret.groups()[1])
                try:
                    license_lst.append(self.general_license_dic[rename_str])
                    continue
                except:
                    pass

        return license_lst


    def __init_licence_pool_info(self, general_dir_name, other_dir_name):
        self.general_lic_path = os.path.join(self.pool_path, general_dir_name)
        self.other_lic_path = os.path.join(self.pool_path, other_dir_name)

        for per_item in os.listdir(self.general_lic_path):
            item_path = os.path.join(self.general_lic_path, per_item)
            related_path = './%s/%s'%(general_dir_name, per_item)
            if os.path.isfile(item_path):
                self.general_license_dic[per_item] = related_path

        for per_item in os.listdir(self.other_lic_path):
            item_path = os.path.join(self.other_lic_path, per_item)
            if os.path.isfile(item_path):
                continue
            related_path = './%s/%s/'%(other_dir_name, per_item)
            self.other_license_dic[per_item] = []
            for subitem in os.listdir(item_path):
                subitem_path = os.path.join(item_path, subitem)
                if os.path.isfile(subitem_path):
                    self.other_license_dic[per_item].append(related_path+subitem)


    def __smart_check(self, pkg_name):
        try:
            return self.license_str[pkg_name.lower()]['license']
        except:
            pass # continue check

        #print 'ERROR: Cannot find package(%s) license.'%pkg_name
        return None

    def __parser_line(self, line_str):
        ret = re.match(r'\s*[Dd]escription\s*:([\s\S]*)', line_str)
        if ret:
            self.in_block = 'description'
            return ('description', ret.groups()[0].strip())

        ret = re.match(r'\s*[Ll]icense\s*:([\s\S]*)', line_str)
        if ret:
            self.in_block = 'license'
            return ('license', ret.groups()[0].strip())

        ret = re.match(r'\s*[vV]ersion\s*:([\s\S]*)', line_str)
        if ret:
            self.in_block = 'version'
            return ('version', ret.groups()[0].strip())

        ret = re.match(r'\s*[Uu][Rr][Ll]\s*:([\s\S]*)', line_str)
        if ret:
            self.in_block = 'url'
            return ('url', ret.groups()[0].strip())

        if self.in_block != '':
            return (None, ' ' + line_str.strip())

        return None


def parse_options():
    if len(sys.argv) < 3:
        print 'ERROR: Invalid parameters. ', sys.argv
        help_info()
        return None
    return sys.argv[1:]


def help_info():
    print 'Example:'
    print '  python build_legal_info.py CONFIG_FILE_PATH LICENSE_PATH CPU_ARCH'


if __name__ == '__main__':
    argv_ret = parse_options()
    if not argv_ret:
        exit(1)
    (config_dir, top_dir, cpu_arch, store_dir)  = argv_ret
    config_file = os.path.join(config_dir, '.config')
    license_dir = os.path.join(top_dir, 'license')
    pkg_dir = os.path.join(top_dir, 'prebuild/third-party', cpu_arch)

    license_pool = LicensePool(license_dir)
    license_pool.import_from_file('software_license_list.txt')
    config_ctn = ConfigContainer(config_file)

    pkg_ctn = PackageContainer(pkg_dir)
    pkg_ctn.scan_and_create(config_ctn, license_pool, store_dir)
