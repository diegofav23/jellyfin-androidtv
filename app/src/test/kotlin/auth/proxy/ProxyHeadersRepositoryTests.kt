package org.jellyfin.androidtv.auth.proxy

import io.kotest.core.spec.style.FunSpec
import io.kotest.matchers.collections.shouldBeEmpty
import io.kotest.matchers.maps.shouldBeEmpty
import io.kotest.matchers.shouldBe
import io.mockk.every
import io.mockk.mockk
import okhttp3.HttpUrl.Companion.toHttpUrl
import org.jellyfin.androidtv.auth.model.AuthenticationStoreServer
import org.jellyfin.androidtv.auth.store.AuthenticationStore
import java.util.UUID

class ProxyHeadersRepositoryTests : FunSpec({
	val headers = mapOf("P-Access-Token-Id" to "id", "P-Access-Token" to "token")

	fun createRepository(vararg servers: AuthenticationStoreServer): ProxyHeadersRepository {
		val store = mockk<AuthenticationStore> {
			every { getServers() } returns servers.associateBy { UUID.randomUUID() }
		}
		return ProxyHeadersRepository(store)
	}

	test("Headers are only returned for the matching host") {
		val repository = createRepository(
			AuthenticationStoreServer(name = "a", address = "https://jellyfin.example.com", proxyHeaders = headers),
			AuthenticationStoreServer(name = "b", address = "https://other.example.com"),
		)

		repository.getHeaders("https://jellyfin.example.com/System/Info/Public".toHttpUrl()) shouldBe headers
		repository.getHeaders("https://JELLYFIN.example.com/Items".toHttpUrl()) shouldBe headers
		repository.getHeaders("https://other.example.com/Items".toHttpUrl()).shouldBeEmpty()
		repository.getHeaders("https://auth.example.com/".toHttpUrl()).shouldBeEmpty()
	}

	test("Headers are not sent over cleartext unless the server uses HTTP") {
		val httpsRepository = createRepository(
			AuthenticationStoreServer(name = "a", address = "https://jellyfin.example.com", proxyHeaders = headers),
		)
		httpsRepository.getHeaders("http://jellyfin.example.com/".toHttpUrl()).shouldBeEmpty()

		val httpRepository = createRepository(
			AuthenticationStoreServer(name = "a", address = "http://jellyfin.lan:8096", proxyHeaders = headers),
		)
		httpRepository.getHeaders("http://jellyfin.lan:8096/".toHttpUrl()) shouldBe headers
	}

	test("Pending headers are used until cleared") {
		val repository = createRepository()
		repository.setPending(
			addresses = listOf("https://jellyfin.example.com", "http://jellyfin.example.com:8096"),
			headers = headers,
			allowCleartext = false,
		)

		repository.getHeaders("https://jellyfin.example.com/".toHttpUrl()) shouldBe headers
		repository.getHeaders("http://jellyfin.example.com:8096/".toHttpUrl()).shouldBeEmpty()
		repository.getAllHeaderNames() shouldBe headers.keys

		repository.clearPending()
		repository.getHeaders("https://jellyfin.example.com/".toHttpUrl()).shouldBeEmpty()
		repository.getAllHeaderNames().shouldBeEmpty()
	}
})
