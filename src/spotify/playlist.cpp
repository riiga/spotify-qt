#include "playlist.hpp"

using namespace spt;

Playlist::Playlist(const QJsonObject &json)
{
	collaborative = json["collaborative"].toBool();
	description = json["description"].toString();
	id = json["id"].toString();
	image = json.contains("image")
		? json["image"].toString()
		: json["images"].toArray()[0].toObject()["url"].toString();
	name = json["name"].toString();
	isPublic = JsonUtils::getProperty(json, {
		"is_public", "public"
	}).toBool();
	tracks = json["tracks"].toObject();
	snapshot = JsonUtils::getProperty(json, {
		"snapshot", "snapshot_id"
	}).toString();
	ownerId = json.contains("owner_id")
		? json["owner_id"].toString()
		: json.contains("ownerId")
			? json["owner_id"].toString()
			: json["owner"].toObject()["id"].toString();
	ownerName = json.contains("owner_name")
		? json["owner_name"].toString()
		: json.contains("ownerName")
			? json["ownerName"].toString()
			: json["owner"].toObject()["display_name"].toString();
}

bool Playlist::loadTracks(Spotify &spotify, std::vector<lib::spt::track> &trackList) const
{
	// Allocate memory for all tracks
	trackList.reserve(tracks["total"].toInt());

	// Load tracks
	auto tracksHref = tracks["href"].toString();
	auto href = QString("%1%2market=from_token")
		.arg(tracksHref)
		.arg(tracksHref.contains('?') ? '&' : '?');

	return loadTracksFromUrl(trackList, href, 0, spotify);
}

std::vector<lib::spt::track> Playlist::loadTracks(Spotify &spotify) const
{
	std::vector<lib::spt::track> trackList;
	loadTracks(spotify, trackList);
	return trackList;
}

bool Playlist::loadTracksFromUrl(std::vector<lib::spt::track> &trackList, QString &url, int offset,
	Spotify &spotify)
{
	// Load tracks from api
	auto newUrl = url.remove(0, QString("https://api.spotify.com/v1/").length());
	auto current = spotify.getAsJson(newUrl);

	// No items, request probably failed
	if (!current.contains("items"))
		return false;

	// Load from url
	auto items = current["items"];
	for (auto &item : items)
		trackList.push_back(item.get<lib::spt::track>());

	// Check if there's a next page
	auto nextPage = current.contains("next") && !current.at("next").is_null()
		? current.at("next").get<std::string>()
		: std::string();
	if (!nextPage.empty())
	{
		auto nextPageUrl = QString::fromStdString(nextPage);
		loadTracksFromUrl(trackList, nextPageUrl, offset + items.size(), spotify);
	}

	return true;
}

QJsonObject Playlist::toJson(Spotify &spotify) const
{
	// Load tracks to put in JSON
	QJsonArray jsonTracks;
	for (auto &track : loadTracks(spotify))
		jsonTracks.append(QString::fromStdString(nlohmann::json(track).dump()));

	return toJson(jsonTracks);
}

bool Playlist::isOwner(const lib::spt::user &user) const
{
	return !ownerId.isEmpty() && ownerId.toStdString() == user.id;
}

QJsonObject Playlist::toJson(const QJsonArray &jsonTracks) const
{
	return QJsonObject({
		QPair<QString, bool>("collaborative", collaborative),
		QPair<QString, QString>("description", description),
		QPair<QString, QString>("id", id),
		QPair<QString, QString>("image", image),
		QPair<QString, QString>("name", name),
		QPair<QString, bool>("is_public", isPublic),
		QPair<QString, int>("total", jsonTracks.size()),
		QPair<QString, QJsonArray>("tracks", jsonTracks),
		QPair<QString, QString>("snapshot", snapshot),
		QPair<QString, QString>("owner_id", ownerId),
		QPair<QString, QString>("owner_name", ownerName)
	});
}
