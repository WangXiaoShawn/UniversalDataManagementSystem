# Full-Stack-C-Project-for-Nationwide-Site-Monitoring
## **Project Introduction**

The **Universal Data Management System** is a powerful automation tool designed to scan specified directories, gather newly generated data, and upload it to the data center using proprietary FTP/TCP protocols. In addition, the data center periodically auto-scans certain directories, scans and organizes files, and incorporates them into the database. All operations are recorded in the log file.

This project is flexible and customizable, able to be quickly modified to support the data types required by clients with only minor changes for rapid deployment. Moreover, the project is extremely cost-effective, utilizing all free versions of components and is capable of handling tens of millions of data per week.

## **How to Run**

The project has been configured with the **`start.sh`** script located in **`~/root/project/idc/c`**. This script automates the scheduling of the entire project's operations.

## **Developer Framework**

I have designed and packaged a developer framework to assist in quick project deployment. Before attempting to replicate my project, please read and understand this developer framework, located at **`~/root/project/public/_public.cpp`**.

## **General Module Overview**

The general modules in the project can be found in the **`/root/project/tools/c`** directory, developed based on the developer module:

### **System Scheduling and Resource Management Module**

- **`procctl`**: A multi-process program scheduling module that is responsible for the timed launch of applications. Once the program starts successfully, it is handed over to the daemon to ensure continuous operation.
- **`checkproc`**: This module uses a shared memory heartbeat mechanism to regularly scan and check the status of all processes. If it detects a process has stopped, it will automatically terminate and restart it, improving system fault tolerance and saving system resources.
- **`gzipfiles`**: This module compresses files. It compresses specified files or directories into gzip format and saves them in a specified directory.
- **`deletefiles`**: As the name implies, this module is used to delete files or directories, providing convenience for system file management.

### **File Transfer Module**

- **`ftpputfiles`**: A file upload module based on the FTP protocol, it supports multi-process uploads, effectively improving efficiency for large-scale file uploads.
- **`ftpgetfiles`**: This is a file download module based on the FTP protocol, supporting incremental and batch modes and multi-process downloads, thus improving file download efficiency.
- **`servers`**: This is a file transfer server based on TCP, used for receiving and sending files.
- **`tcpgetfiles`**: This is a file download system based on the TCP protocol, supporting multi-process batch downloads, used for batch downloading files from the server.
- **`tcpputfiles`**: This is a file upload system based on the TCP protocol, supporting multi-thread batch uploads, used for batch uploading files to the server.

### **Database Management Module**

- **`execsql`**: This is a script file for executing SQL statements, facilitating database queries and modification operations.
- **`dminingmysql`**: This is a dedicated program for the database center, capable of automatically integrating files of various formats and importing data into the database.
- **`yncincrement`**: This is a sub-database system that uses data from the main database to update the sub-database in incremental or batch mode.
- **`deletetable`**: This module is used to completely delete a table from the database.
- **`migratetable`**: This module is used for copying and pasting database tables, which can be used for database backup or migration.

## **Development and Runtime Environment**

Before running this project, ensure that your system is Centro OS 3.7 or a compatible version. If you're using a different operating system, you may need to make some configuration changes to ensure compatibility with this project.

Also, this project uses MySQL 5.7 as its database. Before running or developing this project, you need to make sure that MySQL 5.7 or a compatible version is installed on your system. If you're using a different version of MySQL, you may need to make some configuration changes to ensure compatibility with this project.
