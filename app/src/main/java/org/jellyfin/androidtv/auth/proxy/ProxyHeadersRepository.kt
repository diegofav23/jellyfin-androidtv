package org.jellyfin.androidtv.auth.proxy

import okhttp3.HttpUrl
import okhttp3.HttpUrl.Companion.toHttpUrlOrNull
import org.jellyfin.androidtv.auth.store.AuthenticationStore

/**
 * Keeps track of the additional headers that need to be sent to servers behind an authentication proxy (like Pangolin,
 * Cloudflare Access or Authentik). Headers are matched on the host of the request so they are never sent to other
 * hosts. Headers are only sent over cleartext HTTP when the server address explicitly uses HTTP.
 */
class ProxyHeadersRepository(
	private val authenticationStore: AuthenticationStore,
) {
	private data class Rule(
		val headers: Map<String, String>,
		val allowCleartext: Boolean,
	)

	@Volatile
	private var storedRules: Map<String, Rule>? = null

	@Volatile
	private var pendingRules: Map<String, Rule> = emptyMap()

	/**
	 * Reload the rules from the servers in the [AuthenticationStore].
	 */
	fun refresh() {
		storedRules = authenticationStore.getServers().values
			.toList()
			.filter { server -> server.proxyHeaders.isNotEmpty() }
			.mapNotNull { server ->
				val url = server.address.toHttpUrlOrNull() ?: return@mapNotNull null
				url.host.lowercase() to Rule(server.proxyHeaders, allowCleartext = !url.isHttps)
			}
			.toMap()
	}

	/**
	 * Set headers for a server that is not stored yet (used while adding a new server).
	 */
	fun setPending(addresses: Collection<String>, headers: Map<String, String>, allowCleartext: Boolean) {
		pendingRules = addresses
			.mapNotNull { address -> address.toHttpUrlOrNull()?.host?.lowercase() }
			.associateWith { Rule(headers, allowCleartext) }
	}

	fun clearPending() {
		pendingRules = emptyMap()
	}

	/**
	 * Get the headers to add to a request for [url].
	 */
	fun getHeaders(url: HttpUrl): Map<String, String> {
		val host = url.host.lowercase()
		val rule = pendingRules[host] ?: getStoredRules()[host] ?: return emptyMap()
		if (!url.isHttps && !rule.allowCleartext) return emptyMap()
		return rule.headers
	}

	/**
	 * Get the names of all headers that are known to this repository.
	 */
	fun getAllHeaderNames(): Set<String> = (pendingRules.values + getStoredRules().values)
		.flatMap { rule -> rule.headers.keys }
		.toSet()

	private fun getStoredRules() = storedRules ?: run {
		refresh()
		storedRules.orEmpty()
	}
}
