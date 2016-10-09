# myping

只有root用户才能利用socket()函数生成原始套接字，要让Unix的一般用户能执行以上程序，需进行如下的特别操作：

sudo chown root myping // 把myping属于root用户

sudo chmod u+s myping  // 把myping程序设成SUID的属性。

./myping www.cn.ibm.com


