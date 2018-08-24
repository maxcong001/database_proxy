# canal

## what is canal
The term "Microservice Architecture" has sprung up over the last few years to describe a particular way of designing software applications as suites of independently deployable services. Each service may has needs to access database to make so called "logical separation of data". This will bring the following problem:
1. If each service has a connection to database. There will be too many connections to database.  
2. database is alaways provided by third party company. Most of the time, database is out of software internal network namespace, the communication between service use so called "internal IP", but the database use so called "external IP".
3. when there is some change of database(for example sharding), each service should change accordingly.
4. if each service has connection to database, it is hard to collect over-all database info, such as now many database operation, database traffic, database connection....

canal is a database proxy which provide a proxy function for database. canal acccept connection and dispatch the incoming traffic to database.

Canal has the following feature:
1. fully async.
2. thread pool for connection.
3. service discovery module
4. heartbeat module
5. connection management
6. session affinity
7. DB op performance count
8. Alarm
9. timer module
10. traffic dispatcher module
11. ...


## development record

| time        | change                                                | version |
|-------------|-------------------------------------------------------|---------|
| 2018/08/20  |first commit, ready for demo                           | 0.1     |


## road map

Now in development mode.

Develop for Demo. Will re-write for module programming.
