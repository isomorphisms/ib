package org.isomorphisms.ib.material3

import android.content.res.AssetManager
import android.os.Bundle
import android.os.SystemClock
import android.util.Log
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.heightIn
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.weight
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.LazyListState
import androidx.compose.foundation.lazy.items
import androidx.compose.foundation.lazy.rememberLazyListState
import androidx.compose.foundation.text.selection.SelectionContainer
import androidx.compose.material3.HorizontalDivider
import androidx.compose.material3.ListItem
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Scaffold
import androidx.compose.material3.Surface
import androidx.compose.material3.Text
import androidx.compose.material3.TopAppBar
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Modifier
import androidx.compose.ui.unit.dp
import java.io.File
import java.io.IOException
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.NonCancellable
import kotlinx.coroutines.delay
import kotlinx.coroutines.isActive
import kotlinx.coroutines.withContext
import kotlin.math.max
import kotlin.math.min

private const val exposureSampleMilliseconds = 500L
private const val exposureFlushSamples = 4
private const val logTag = "IBViewExposure"

class MainActivity : ComponentActivity() {
    private lateinit var exposureStore: ViewExposure.Store
    private val readerVisible = mutableStateOf(false)

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        val articles = loadPensieveArticles(assets)
        exposureStore = ViewExposure.Store(File(filesDir, "pensieve/view-history"))
        setContent {
            MaterialTheme {
                ArxivViewport(articles, exposureStore, readerVisible.value)
            }
        }
    }

    override fun onStart() {
        super.onStart()
        readerVisible.value = true
    }

    override fun onStop() {
        readerVisible.value = false
        try {
            exposureStore.flush()
        } catch (error: IOException) {
            Log.w(logTag, "Could not flush local view exposure", error)
        }
        super.onStop()
    }
}

private data class PensieveArticle(
    val arxivId: String,
    val title: String,
    val body: String?,
    val textSource: String?,
    val fragments: List<ViewExposure.Fragment>,
)

private fun readAssetOrNull(assets: AssetManager, path: String): String? =
    try {
        assets.open(path).bufferedReader().use { it.readText() }
    } catch (_: IOException) {
        null
    }

private fun loadPensieveArticles(assets: AssetManager): List<PensieveArticle> {
    val root = "pensieve/arxiv"
    val identifiers = assets.list(root)
        ?.filter { it.isNotBlank() }
        ?.sorted()
        .orEmpty()

    return identifiers.map { identifier ->
        val itemRoot = "$root/$identifier"
        val title = readAssetOrNull(assets, "$itemRoot/title")
            ?.trim()
            ?.takeIf { it.isNotEmpty() }
            ?: "arXiv $identifier"

        val textCandidates = listOf(
            "text/from-pdf.txt",
            "text/from-html.txt",
            "text/from-abstract.txt",
        )

        var body: String? = null
        var textSource: String? = null
        for (candidate in textCandidates) {
            val text = readAssetOrNull(assets, "$itemRoot/$candidate")
                ?.trim()
                ?.takeIf { it.isNotEmpty() }
            if (text != null) {
                body = text
                textSource = candidate
                break
            }
        }

        PensieveArticle(
            arxivId = identifier,
            title = title,
            body = body,
            textSource = textSource,
            fragments = body?.let { ViewExposure.fragmentsForArticle(identifier, it) }.orEmpty(),
        )
    }
}

@Composable
private fun ArxivViewport(
    articles: List<PensieveArticle>,
    exposureStore: ViewExposure.Store,
    readerVisible: Boolean,
) {
    var selected by remember(articles) { mutableStateOf(articles.firstOrNull()) }
    val readerState = rememberLazyListState()

    LaunchedEffect(selected?.arxivId) {
        readerState.scrollToItem(0)
    }
    RecordViewExposure(selected, readerState, exposureStore, readerVisible)

    Scaffold(
        topBar = {
            TopAppBar(
                title = { Text("IB") },
            )
        },
    ) { scaffoldPadding ->
        Column(
            modifier = Modifier
                .fillMaxSize()
                .padding(scaffoldPadding),
        ) {
            Text(
                text = "Pensieve · ${articles.size} arXiv entries",
                style = MaterialTheme.typography.labelLarge,
                modifier = Modifier.padding(horizontal = 16.dp, vertical = 8.dp),
            )

            LazyColumn(
                modifier = Modifier
                    .fillMaxWidth()
                    .heightIn(max = 260.dp),
            ) {
                items(
                    items = articles,
                    key = { article -> article.arxivId },
                ) { article ->
                    ListItem(
                        headlineContent = { Text(article.title) },
                        supportingContent = {
                            Text("Pensieve · arxiv/${article.arxivId}")
                        },
                        modifier = Modifier.clickable { selected = article },
                    )
                    HorizontalDivider()
                }
            }

            Surface(
                modifier = Modifier
                    .fillMaxWidth()
                    .weight(1f)
                    .padding(12.dp),
                shape = MaterialTheme.shapes.large,
                tonalElevation = 1.dp,
            ) {
                SelectionContainer {
                    ArticleReader(selected, readerState)
                }
            }
        }
    }
}

@Composable
private fun ArticleReader(
    article: PensieveArticle?,
    readerState: LazyListState,
) {
    LazyColumn(
        state = readerState,
        modifier = Modifier
            .fillMaxSize()
            .padding(18.dp),
    ) {
        if (article == null) {
            item(key = "empty-pensieve") {
                Text(
                    text = "The Pensieve snapshot is empty.",
                    style = MaterialTheme.typography.bodyLarge,
                )
            }
            return@LazyColumn
        }

        item(key = "heading:${article.arxivId}") {
            Text(
                text = article.title,
                style = MaterialTheme.typography.headlineSmall,
            )
            Text(
                text = "arXiv ${article.arxivId}",
                style = MaterialTheme.typography.bodyMedium,
            )
            Text(
                text = article.textSource?.let {
                    "Pensieve · arxiv/${article.arxivId}/$it"
                } ?: "Pensieve · arxiv/${article.arxivId}",
                style = MaterialTheme.typography.labelMedium,
            )
            Spacer(modifier = Modifier.height(18.dp))
        }

        if (article.fragments.isEmpty()) {
            item(key = "no-text:${article.arxivId}") {
                Text(
                    text = "No text representation is present in this frozen Pensieve snapshot.",
                    style = MaterialTheme.typography.bodyLarge,
                )
            }
        } else {
            items(
                items = article.fragments,
                key = { fragment -> fragment.id },
            ) { fragment ->
                Text(
                    text = fragment.text,
                    style = MaterialTheme.typography.bodyLarge,
                    modifier = Modifier
                        .fillMaxWidth()
                        .padding(vertical = 6.dp),
                )
            }
        }
    }
}

@Composable
private fun RecordViewExposure(
    article: PensieveArticle?,
    readerState: LazyListState,
    exposureStore: ViewExposure.Store,
    readerVisible: Boolean,
) {
    val fragmentById = remember(article) {
        article?.fragments?.associateBy { it.id }.orEmpty()
    }

    LaunchedEffect(article?.arxivId, article?.textSource, readerState, exposureStore, readerVisible) {
        if (!readerVisible) {
            return@LaunchedEffect
        }
        val session = ViewExposure.Session()
        var samplesSinceFlush = 0
        try {
            while (isActive) {
                val snapshot = currentViewportSnapshot(readerState, fragmentById)
                val observations = session.observe(
                    System.currentTimeMillis(),
                    SystemClock.elapsedRealtime(),
                    snapshot,
                )
                exposureStore.record(observations)
                samplesSinceFlush += 1
                if (samplesSinceFlush >= exposureFlushSamples) {
                    flushExposure(exposureStore)
                    samplesSinceFlush = 0
                }
                delay(exposureSampleMilliseconds)
            }
        } finally {
            withContext(NonCancellable + Dispatchers.IO) {
                try {
                    exposureStore.flush()
                } catch (error: IOException) {
                    Log.w(logTag, "Could not flush local view exposure", error)
                }
            }
        }
    }
}

private suspend fun flushExposure(exposureStore: ViewExposure.Store) {
    withContext(Dispatchers.IO) {
        try {
            exposureStore.flush()
        } catch (error: IOException) {
            Log.w(logTag, "Could not flush local view exposure", error)
        }
    }
}

private fun currentViewportSnapshot(
    readerState: LazyListState,
    fragmentById: Map<String, ViewExposure.Fragment>,
): ViewExposure.ViewportSnapshot {
    val layout = readerState.layoutInfo
    val viewportStart = layout.viewportStartOffset
    val viewportEnd = layout.viewportEndOffset
    val viewportHeight = max(1, viewportEnd - viewportStart)
    val visible = layout.visibleItemsInfo.mapNotNull { item ->
        val fragmentId = item.key as? String ?: return@mapNotNull null
        if (!fragmentById.containsKey(fragmentId)) {
            return@mapNotNull null
        }
        val visibleStart = max(item.offset, viewportStart)
        val visibleEnd = min(item.offset + item.size, viewportEnd)
        val visiblePixels = max(0, visibleEnd - visibleStart)
        if (visiblePixels == 0) {
            return@mapNotNull null
        }
        ViewExposure.VisibleFragment(
            fragmentId,
            item.offset,
            visiblePixels,
            item.size,
        )
    }

    return ViewExposure.ViewportSnapshot(
        visible,
        readerState.firstVisibleItemIndex,
        readerState.firstVisibleItemScrollOffset,
        viewportHeight,
    )
}
