#include <drogon/drogon.h>

using namespace drogon;
using Callback = std::function<void(const HttpResponsePtr &)>;

int main()
{
    drogon::app().loadConfigFile("config.json");
    
    app().registerHandler("/users",
        [](const HttpRequestPtr &req, Callback &&callback) {
            auto client = drogon::app().getDbClient("postgres");
            client->execSqlAsync(
                "SELECT id, name, email FROM users ORDER BY id",
                [callback](const drogon::orm::Result &result) {
                    Json::Value users(Json::arrayValue);
                    for (const auto &row : result) {
                        Json::Value user;
                        user["id"] = row["id"].as<int>();
                        user["name"] = row["name"].as<std::string>();
                        user["email"] = row["email"].as<std::string>();
                        users.append(user);
                    }
                    auto resp = HttpResponse::newHttpJsonResponse(users);
                    callback(resp);
                },
                [callback](const drogon::orm::DrogonDbException &err) {
                    Json::Value error;
                    error["error"] = err.base().what();
                    callback(HttpResponse::newHttpJsonResponse(error));
                });
        },
        {Get});

    app().registerHandler("/users/{id}",
        [](const HttpRequestPtr &req, Callback &&callback, int id) {
            auto client = drogon::app().getDbClient("postgres");
            client->execSqlAsync(
                "SELECT id, name, email FROM users WHERE id = $1",
                [callback](const drogon::orm::Result &result) {
                    if (result.empty()) {
                        auto resp = HttpResponse::newHttpResponse();
                        resp->setStatusCode(k404NotFound);
                        callback(resp);
                        return;
                    }
                    const auto &row = result[0];
                    Json::Value user;
                    user["id"] = row["id"].as<int>();
                    user["name"] = row["name"].as<std::string>();
                    user["email"] = row["email"].as<std::string>();
                    callback(HttpResponse::newHttpJsonResponse(user));
                },
                [callback](const drogon::orm::DrogonDbException &err) {
                    Json::Value error;
                    error["error"] = err.base().what();
                    callback(HttpResponse::newHttpJsonResponse(error));
                },
                id);
        },
        {Get});

    app().registerHandler("/users",
        [](const HttpRequestPtr &req, Callback &&callback) {
            auto body = req->getJsonObject();
            if (!body || (*body)["name"].isNull() || (*body)["email"].isNull()) {
                Json::Value error;
                error["error"] = "name and email are required";
                auto resp = HttpResponse::newHttpJsonResponse(error);
                resp->setStatusCode(k400BadRequest);
                callback(resp);
                return;
            }
            auto client = drogon::app().getDbClient("postgres");
            std::string name = (*body)["name"].asString();
            std::string email = (*body)["email"].asString();
            client->execSqlAsync(
                "INSERT INTO users (name, email) VALUES ($1, $2) RETURNING id, name, email",
                [callback](const drogon::orm::Result &result) {
                    const auto &row = result[0];
                    Json::Value user;
                    user["id"] = row["id"].as<int>();
                    user["name"] = row["name"].as<std::string>();
                    user["email"] = row["email"].as<std::string>();
                    auto resp = HttpResponse::newHttpJsonResponse(user);
                    resp->setStatusCode(k201Created);
                    callback(resp);
                },
                [callback](const drogon::orm::DrogonDbException &err) {
                    Json::Value error;
                    error["error"] = err.base().what();
                    callback(HttpResponse::newHttpJsonResponse(error));
                },
                name, email);
        },
        {Post});

    app().registerHandler("/users/{id}",
        [](const HttpRequestPtr &req, Callback &&callback, int id) {
            auto body = req->getJsonObject();
            if (!body) {
                Json::Value error;
                error["error"] = "Invalid JSON body";
                auto resp = HttpResponse::newHttpJsonResponse(error);
                resp->setStatusCode(k400BadRequest);
                callback(resp);
                return;
            }
            auto client = drogon::app().getDbClient("postgres");
            std::string name = (*body)["name"].asString();
            std::string email = (*body)["email"].asString();
            client->execSqlAsync(
                "UPDATE users SET name = $1, email = $2 WHERE id = $3 RETURNING id, name, email",
                [callback](const drogon::orm::Result &result) {
                    if (result.empty()) {
                        auto resp = HttpResponse::newHttpResponse();
                        resp->setStatusCode(k404NotFound);
                        callback(resp);
                        return;
                    }
                    const auto &row = result[0];
                    Json::Value user;
                    user["id"] = row["id"].as<int>();
                    user["name"] = row["name"].as<std::string>();
                    user["email"] = row["email"].as<std::string>();
                    callback(HttpResponse::newHttpJsonResponse(user));
                },
                [callback](const drogon::orm::DrogonDbException &err) {
                    Json::Value error;
                    error["error"] = err.base().what();
                    callback(HttpResponse::newHttpJsonResponse(error));
                },
                name, email, id);
        },
        {Put});

    app().registerHandler("/users/{id}",
        [](const HttpRequestPtr &req, Callback &&callback, int id) {
            auto client = drogon::app().getDbClient("postgres");
            client->execSqlAsync(
                "DELETE FROM users WHERE id = $1",
                [callback](const drogon::orm::Result &result) {
                    auto resp = HttpResponse::newHttpResponse();
                    if (result.empty()) {
                        resp->setStatusCode(k404NotFound);
                    } else {
                        resp->setStatusCode(k204NoContent);
                    }
                    callback(resp);
                },
                [callback](const drogon::orm::DrogonDbException &err) {
                    Json::Value error;
                    error["error"] = err.base().what();
                    callback(HttpResponse::newHttpJsonResponse(error));
                },
                id);
        },
        {Delete});

    app().run();
}
